/*
 * device.cpp - NeoTrellis device implementation for iii
 *
 * Implements device.h interface for the dinkii-neotrellis hardware.
 * Supports two modes stored in flash:
 *   mode 0 (iii)    - Lua scripting via iii, grid events dispatched to event_grid()
 *   mode 1 (monome) - standard monome serial protocol over USB CDC
 */

// C++ headers first (cannot be inside extern "C")
#include "MonomeSerialDevice.h"
#include "Adafruit_seesaw/Adafruit_NeoTrellis.h"

// C headers from iii — wrap in extern "C" so C++ sees them with C linkage
extern "C" {
#include "device.h"
#include "vm.h"
#include "serial.h"
#include "util.h"
}

// Lua C API (lua.h already has its own extern "C" guards)
#include "lua.h"
#include "lauxlib.h"

#include "config.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <string.h>

// ---------------------------------------------------------------------------
// mode — defined in dinkii-neotrellis.cpp, set before device_init() is called
// ---------------------------------------------------------------------------
extern uint8_t mode;

// ---------------------------------------------------------------------------
// NeoTrellis objects
// ---------------------------------------------------------------------------

#if GRIDCOUNT == SIXTEEN
Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    {Adafruit_NeoTrellis(addrRowOne[1])}};
#endif

#if GRIDCOUNT == SIXTYFOUR
Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    {Adafruit_NeoTrellis(addrRowOne[0]), Adafruit_NeoTrellis(addrRowOne[1])},
    {Adafruit_NeoTrellis(addrRowTwo[0]), Adafruit_NeoTrellis(addrRowTwo[1])}};
#endif

#if GRIDCOUNT == ONETWENTEIGHT
Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    {Adafruit_NeoTrellis(addrRowOne[0]), Adafruit_NeoTrellis(addrRowOne[1]),
     Adafruit_NeoTrellis(addrRowOne[2]), Adafruit_NeoTrellis(addrRowOne[3])},
    {Adafruit_NeoTrellis(addrRowTwo[0]), Adafruit_NeoTrellis(addrRowTwo[1]),
     Adafruit_NeoTrellis(addrRowTwo[2]), Adafruit_NeoTrellis(addrRowTwo[3])}};
#endif

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)trellis_array,
                              NUM_ROWS / 4, NUM_COLS / 4);

// ---------------------------------------------------------------------------
// MonomeSerialDevice — used in mode 1 (monome protocol)
// ---------------------------------------------------------------------------
MonomeSerialDevice mdp;
static int prevLedBuffer[MonomeSerialDevice::MAXLEDCOUNT];

// ---------------------------------------------------------------------------
// Grid LED state for mode 0 (iii) — 0–15 intensity per cell
// ---------------------------------------------------------------------------
static uint8_t local_leds[NUM_ROWS * NUM_COLS]; // current LED levels
static uint8_t mmap[NUM_ROWS * NUM_COLS];        // mirror for relative ops
static bool    grid_dirty = false;

// ---------------------------------------------------------------------------
// Key event buffer — populated by keyCallback, drained in device_task()
// ---------------------------------------------------------------------------
#define GRID_EVENTS_MAX 64
static uint8_t grid_event_count = 0;
static uint8_t grid_event_rptr  = 0;

struct grid_event_t { uint8_t x, y, z; };
static grid_event_t grid_events[GRID_EVENTS_MAX];

static void grid_add_event(uint8_t x, uint8_t y, uint8_t z) {
    if (grid_event_count < GRID_EVENTS_MAX) {
        grid_events[grid_event_count].x = x;
        grid_events[grid_event_count].y = y;
        grid_events[grid_event_count].z = z;
        grid_event_count++;
    }
}

// ---------------------------------------------------------------------------
// LED update helpers
// ---------------------------------------------------------------------------

// Convert a 0–15 level to a NeoTrellis RGB colour using config.h gamma + RGB
static inline uint32_t level_to_color(uint8_t val) {
    uint8_t gval = (uint8_t)((uint16_t)gammaTable[val] * gammaAdj);
    return (((uint32_t)((uint16_t)gval * R) / 256) << 16)
         | (((uint32_t)((uint16_t)gval * G) / 256) << 8)
         |  ((uint32_t)((uint16_t)gval * B) / 256);
}

// Push local_leds[] (0–15 values) to NeoTrellis — used in iii mode
static void sendLeds_iii() {
    for (int i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        trellis.setPixelColor(i, level_to_color(local_leds[i]));
    }
    trellis.show();
}

// Push mdp.leds[] (0–15 values) to NeoTrellis — used in monome mode
static void sendLeds_monome() {
    bool dirty = false;
    for (int i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        uint8_t val = mdp.leds[i];
        if (val != (uint8_t)prevLedBuffer[i]) {
            trellis.setPixelColor(i, level_to_color(val));
            prevLedBuffer[i] = val;
            dirty = true;
        }
    }
    if (dirty) trellis.show();
}

// ---------------------------------------------------------------------------
// NeoTrellis key callback — dispatches based on current mode
// ---------------------------------------------------------------------------
static TrellisCallback keyCallback(keyEvent evt) {
    uint8_t x = evt.bit.NUM % NUM_COLS;
    uint8_t y = evt.bit.NUM / NUM_COLS;
    uint8_t z = (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) ? 1 : 0;

    if (mode == 0) {
        // iii mode: buffer the event; device_task() will call vm_handle_grid_key
        grid_add_event(x, y, z);
    } else {
        // monome mode: send key event directly via serial protocol
        mdp.sendGridKey(x, y, z);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// vm_handle_grid_key — fire Lua event_grid(x, y, z)
// ---------------------------------------------------------------------------
static void vm_handle_grid_key(uint8_t x, uint8_t y, uint8_t z) {
    if (L == NULL) return;
    lua_getglobal(L, "event_grid");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    lua_pushinteger(L, x + 1); // Lua is 1-based
    lua_pushinteger(L, y + 1);
    lua_pushinteger(L, z);
    l_report(L, docall(L, 3, 0));
}

// ---------------------------------------------------------------------------
// device.h implementations (extern "C" so C code in iii can link against them)
// ---------------------------------------------------------------------------

// Returns the I2C address of the first (top-left) seesaw tile.
// This is the tile that covers grid keys (0,0)–(3,3).
static uint8_t first_tile_addr() {
#if GRIDCOUNT == SIXTYFOUR || GRIDCOUNT == ONETWENTEIGHT
    return addrRowOne[0];
#else
    return NEO_TRELLIS_ADDR; // 0x2E — default for SIXTEEN / TWOFIFTYSIX
#endif
}

// Check whether grid key (0,0) is pressed during boot.
// Used by main() to decide whether to toggle the stored device mode.
//
// Initialises only the first seesaw tile, enables a key-pressed event for key
// (0,0), then polls for up to 500 ms.  The tile will be fully re-initialised
// by device_init() afterwards, so partial state here is harmless.
extern "C" bool check_device_key() {
    // Bring up just the first tile (also initialises I2C via Adafruit_I2CDevice)
    trellis_array[0][0].begin(first_tile_addr());

    // NEO_TRELLIS_KEY(0) == 0: the seesaw key number for tile-local key 0,
    // which corresponds to grid position (0,0).
    trellis_array[0][0].setKeypadEvent(NEO_TRELLIS_KEY(0),
                                       SEESAW_KEYPAD_EDGE_FALLING, true);

    // Poll for up to 500 ms; return true on first detected press
    for (int i = 0; i < 50; i++) {
        sleep_ms(10);
        if (trellis_array[0][0].getKeypadCount() > 0) return true;
    }
    return false;
}

// Initialize NeoTrellis hardware (called before iii_loop or device_monome_loop)
extern "C" void device_init() {
    trellis.begin();

    // Register key callbacks for all cells
    for (uint8_t x = 0; x < NUM_COLS; x++) {
        for (uint8_t y = 0; y < NUM_ROWS; y++) {
            trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
            trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
            trellis.registerCallback(x, y, keyCallback);
        }
    }

    // Set overall pixel brightness
    for (uint8_t x = 0; x < NUM_COLS / 4; x++) {
        for (uint8_t y = 0; y < NUM_ROWS / 4; y++) {
            trellis_array[y][x].pixels.setBrightness(BRIGHTNESS);
        }
    }

    // Clear LED state
    memset(local_leds,   0, sizeof(local_leds));
    memset(mmap,         0, sizeof(mmap));
    memset(prevLedBuffer, 0, sizeof(prevLedBuffer));
    sendLeds_iii(); // push zeros to hardware

    // Single startup blink on key 0
    trellis.setPixelColor(0, 0xFFFFFF);
    trellis.show();
    sleep_ms(100);
    trellis.setPixelColor(0, 0x000000);
    trellis.show();
}

// Called from iii_loop's while(1) — handles trellis reads and Lua key dispatch
extern "C" void device_task() {
    // Throttle to ~60 Hz
    static uint32_t last_refresh = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_refresh < 16) return;
    last_refresh = now;

    trellis.read(); // fires keyCallback for any pending key events

    // Drain key event buffer → fire Lua event_grid for each
    while (grid_event_rptr < grid_event_count) {
        vm_handle_grid_key(grid_events[grid_event_rptr].x,
                           grid_events[grid_event_rptr].y,
                           grid_events[grid_event_rptr].z);
        grid_event_rptr++;
    }
    if (grid_event_rptr == grid_event_count) {
        grid_event_count = 0;
        grid_event_rptr  = 0;
    }

    // Flush LED changes requested by Lua grid_refresh()
    if (grid_dirty) {
        sendLeds_iii();
        grid_dirty = false;
    }
}

// Not used by this device — serial in iii mode is handled by the REPL
extern "C" void device_handle_serial(uint8_t *data, uint32_t len) {
    (void)data;
    (void)len;
}

// ---------------------------------------------------------------------------
// Lua grid bindings — registered via get_device_lib() → vm_init()
// ---------------------------------------------------------------------------

static int l_grid_led(lua_State *l) {
    // grid_led(x, y, z [, rel])  — x/y are 1-based; z is 0–15
    uint8_t x   = (uint8_t)lua_tointeger(l, 1) - 1;
    uint8_t y   = (uint8_t)lua_tointeger(l, 2) - 1;
    int8_t  z   = (int8_t) lua_tointeger(l, 3);
    bool    rel = lua_toboolean(l, 4);
    if (x >= NUM_COLS || y >= NUM_ROWS) return 0;
    int idx = y * NUM_COLS + x;
    if (rel) {
        z = (int8_t)clamp((int)z + (int)mmap[idx], 0, 15);
    } else {
        z = (int8_t)clamp((int)z, 0, 15);
    }
    local_leds[idx] = (uint8_t)z;
    mmap[idx]       = (uint8_t)z;
    return 0;
}

static int l_grid_led_get(lua_State *l) {
    uint8_t x = (uint8_t)lua_tointeger(l, 1) - 1;
    uint8_t y = (uint8_t)lua_tointeger(l, 2) - 1;
    x = x % NUM_COLS;
    y = y % NUM_ROWS;
    lua_pushinteger(l, mmap[y * NUM_COLS + x]);
    return 1;
}

static int l_grid_led_all(lua_State *l) {
    int8_t z   = (int8_t)lua_tointeger(l, 1);
    bool   rel = lua_toboolean(l, 2);
    if (rel) {
        for (int i = 0; i < NUM_ROWS * NUM_COLS; i++) {
            int8_t zz = (int8_t)clamp((int)z + (int)mmap[i], 0, 15);
            mmap[i]       = (uint8_t)zz;
            local_leds[i] = (uint8_t)zz;
        }
    } else {
        z = (int8_t)clamp((int)z, 0, 15);
        memset(mmap,       (uint8_t)z, sizeof(mmap));
        memset(local_leds, (uint8_t)z, sizeof(local_leds));
    }
    return 0;
}

static int l_grid_intensity(lua_State *l) {
    // Scales the per-pixel brightness via the NeoPixel brightness register
    uint8_t z = (uint8_t)lua_tointeger(l, 1);
    if (z > 15) z = 15;
    // Map 0–15 to 0–255 brightness
    uint8_t brightness = (uint8_t)((uint16_t)z * 255 / 15);
    for (uint8_t x = 0; x < NUM_COLS / 4; x++) {
        for (uint8_t y = 0; y < NUM_ROWS / 4; y++) {
            trellis_array[y][x].pixels.setBrightness(brightness);
        }
    }
    grid_dirty = true;
    return 0;
}

static int l_grid_refresh(lua_State *l) {
    (void)l;
    grid_dirty = true;
    return 0;
}

static int l_grid_size_x(lua_State *l) {
    lua_pushinteger(l, NUM_COLS);
    return 1;
}

static int l_grid_size_y(lua_State *l) {
    lua_pushinteger(l, NUM_ROWS);
    return 1;
}

static const luaL_Reg device_lib[] = {
    {"grid_led",       l_grid_led},
    {"grid_led_get",   l_grid_led_get},
    {"grid_led_all",   l_grid_led_all},
    {"grid_intensity", l_grid_intensity},
    {"grid_refresh",   l_grid_refresh},
    {"grid_size_x",    l_grid_size_x},
    {"grid_size_y",    l_grid_size_y},
    {NULL, NULL}
};

extern "C" const luaL_Reg *get_device_lib(void) {
    return device_lib;
}

// ---------------------------------------------------------------------------
// Device info strings
// ---------------------------------------------------------------------------

static const char *device_help_str =
    "grid\n"
    "  event_grid(x,y,z)\n"
    "  grid_led(x,y,z,rel)\n"
    "  grid_led_get(x,y)\n"
    "  grid_led_all(z,rel)\n"
    "  grid_intensity(z)\n"
    "  grid_refresh()\n"
    "  grid_size_x()\n"
    "  grid_size_y()\n";

extern "C" const char *device_help_txt() { return device_help_str; }
extern "C" const char *device_id()       { return "grid"; }
extern "C" const char *device_str1()     { return "monome"; }
extern "C" const char *device_str2()     { return (mode == 0) ? "iii grid" : "grid"; }

// ---------------------------------------------------------------------------
// Monome mode main loop — called from main() when mode == 1
// Handles the standard monome serial protocol via MonomeSerialDevice
// ---------------------------------------------------------------------------
extern "C" void device_monome_loop() {
    mdp.isMonome = true;
    mdp.deviceID = deviceID;
    mdp.setupAsGrid(NUM_ROWS, NUM_COLS);

    // Brief USB handshake window
    for (int i = 0; i < 8; i++) {
        tud_task();
        mdp.poll();
        sleep_ms(100);
    }
    mdp.getDeviceInfo();

    // Clear LEDs
    mdp.setAllLEDs(0);
    sendLeds_monome();

    uint32_t last_refresh = to_ms_since_boot(get_absolute_time());

    while (true) {
        tud_task();
        mdp.poll(); // read incoming monome protocol, update mdp.leds[]

        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_refresh >= 16) {
            trellis.read();       // fires keyCallback → mdp.sendGridKey
            sendLeds_monome();    // push any LED changes from host
            last_refresh = now;
        }
        tud_cdc_write_flush();
    }
}
