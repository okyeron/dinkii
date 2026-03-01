/*
 * device.cpp - NeoTrellis device implementation for iii
 *
 * Implements device.h interface for the dinkii-neotrellis hardware.
 * Supports two modes stored in flash:
 *   mode 0 (iii)    - Lua scripting via iii, grid events dispatched to event_grid()
 *   mode 1 (monome) - standard monome serial protocol over USB CDC
 *
 * Lua API calls (lua_pushinteger, lua_tointeger, etc.) live in device_lua.c,
 * which is compiled WITHOUT LUA_USE_C89.  This file contains only hardware
 * interaction and a plain-C LED API that device_lua.c calls back into.
 */

// C++ headers first (cannot be inside extern "C")
#include "MonomeSerialDevice.h"
#include "Adafruit_seesaw/Adafruit_NeoTrellis.h"

// C headers from iii — wrap in extern "C" so C++ sees them with C linkage
extern "C" {
#include "device.h"
#include "device_ext.h"
#include "serial.h"
#include "util.h"
}

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
static uint8_t local_leds[NUM_ROWS * NUM_COLS];
static uint8_t mmap[NUM_ROWS * NUM_COLS];
static bool    grid_dirty = false;

// ---------------------------------------------------------------------------
// Key event buffer — populated by keyCallback, drained in device_task()
// ---------------------------------------------------------------------------
#define GRID_EVENTS_MAX 64
static uint8_t grid_event_count = 0;
static struct {
    uint8_t x;
    uint8_t y;
    uint8_t z;
} grid_events[GRID_EVENTS_MAX];

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

static inline uint32_t level_to_color(uint8_t val) {
    uint8_t gval = (uint8_t)((uint16_t)gammaTable[val] * gammaAdj);
    return (((uint32_t)((uint16_t)gval * R) / 256) << 16)
         | (((uint32_t)((uint16_t)gval * G) / 256) << 8)
         |  ((uint32_t)((uint16_t)gval * B) / 256);
}

static void sendLeds_iii() {
    for (int i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        trellis.setPixelColor(i, level_to_color(local_leds[i]));
    }
    trellis.show();
}

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
        grid_add_event(x, y, z);
    } else {
        mdp.sendGridKey(x, y, z);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// device.h implementations (extern "C" so C code in iii can link against them)
// ---------------------------------------------------------------------------

static uint8_t first_tile_addr() {
#if GRIDCOUNT == SIXTYFOUR || GRIDCOUNT == ONETWENTEIGHT
    return addrRowOne[0];
#else
    return NEO_TRELLIS_ADDR;
#endif
}

extern "C" bool check_device_key() {
    trellis_array[0][0].begin(first_tile_addr());
    trellis_array[0][0].setKeypadEvent(NEO_TRELLIS_KEY(0),
                                       SEESAW_KEYPAD_EDGE_FALLING, true);
    for (int i = 0; i < 50; i++) {
        sleep_ms(10);
        if (trellis_array[0][0].getKeypadCount() > 0) return true;
    }
    return false;
}

extern "C" void device_init() {
    trellis.begin();

    for (uint8_t x = 0; x < NUM_COLS; x++) {
        for (uint8_t y = 0; y < NUM_ROWS; y++) {
            trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
            trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
            trellis.registerCallback(x, y, keyCallback);
        }
    }

    for (uint8_t x = 0; x < NUM_COLS / 4; x++) {
        for (uint8_t y = 0; y < NUM_ROWS / 4; y++) {
            trellis_array[y][x].pixels.setBrightness(BRIGHTNESS);
        }
    }

    memset(local_leds,   0, sizeof(local_leds));
    memset(mmap,         0, sizeof(mmap));
    memset(prevLedBuffer, 0, sizeof(prevLedBuffer));
    sendLeds_iii();

    gpio_put(LED_PIN, 0);

    trellis.setPixelColor(0, 0xFFFFFF);
    trellis.show();
    sleep_ms(100);
    trellis.setPixelColor(0, 0x000000);
    trellis.show();
}

extern "C" void device_task() {
    static uint32_t last_refresh = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_refresh < 16) return;
    last_refresh = now;

    trellis.read();

    static uint8_t ewr = 0;
    if (grid_event_count > 0) {
        vm_handle_grid_key(grid_events[ewr].x, grid_events[ewr].y, grid_events[ewr].z);
        ewr++;
        if (ewr == grid_event_count) {
            grid_event_count = 0;
            ewr = 0;
        }
    }

    if (grid_dirty) {
        sendLeds_iii();
        grid_dirty = false;
    }
}

extern "C" void device_handle_serial(uint8_t *data, uint32_t len) {
    (void)data;
    (void)len;
}

// ---------------------------------------------------------------------------
// LED C API — called from device_lua.c (compiled without LUA_USE_C89)
// ---------------------------------------------------------------------------

extern "C" void device_led_set(int x, int y, int z, int rel) {
    if (x < 0 || x >= NUM_COLS || y < 0 || y >= NUM_ROWS) return;
    int idx = y * NUM_COLS + x;
    int8_t z8 = (int8_t)(rel ? clamp(z + (int)mmap[idx], 0, 15)
                              : clamp(z, 0, 15));
    local_leds[idx] = (uint8_t)z8;
    mmap[idx]       = (uint8_t)z8;
    grid_dirty = true;
}

extern "C" int device_led_get(int x, int y) {
    x = ((x % NUM_COLS) + NUM_COLS) % NUM_COLS;
    y = ((y % NUM_ROWS) + NUM_ROWS) % NUM_ROWS;
    return (int)mmap[y * NUM_COLS + x];
}

extern "C" void device_led_all(int z, int rel) {
    if (rel) {
        for (int i = 0; i < NUM_ROWS * NUM_COLS; i++) {
            int8_t zz = (int8_t)clamp(z + (int)mmap[i], 0, 15);
            mmap[i]       = (uint8_t)zz;
            local_leds[i] = (uint8_t)zz;
        }
    } else {
        uint8_t z8 = (uint8_t)clamp(z, 0, 15);
        memset(mmap,       z8, sizeof(mmap));
        memset(local_leds, z8, sizeof(local_leds));
    }
    grid_dirty = true;
}

extern "C" void device_intensity(int z) {
    if (z > 15) z = 15;
    uint8_t brightness = (uint8_t)((z * 255) / 15);
    for (uint8_t xi = 0; xi < NUM_COLS / 4; xi++) {
        for (uint8_t yi = 0; yi < NUM_ROWS / 4; yi++) {
            trellis_array[yi][xi].pixels.setBrightness(brightness);
        }
    }
    grid_dirty = true;
}

extern "C" void device_mark_dirty(void) { grid_dirty = true; }
extern "C" int  device_cols(void)       { return NUM_COLS; }
extern "C" int  device_rows(void)       { return NUM_ROWS; }

// ---------------------------------------------------------------------------
// Device info strings
// ---------------------------------------------------------------------------

static const char *device_help_str =
    "grid\n"
    "  event_grid(x,y,z)  (alias: grid)\n"
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
// Monome mode main loop
// ---------------------------------------------------------------------------
extern "C" void device_monome_loop() {
    mdp.isMonome = true;
    mdp.deviceID = deviceID;
    mdp.setupAsGrid(NUM_ROWS, NUM_COLS);

    for (int i = 0; i < 8; i++) {
        tud_task();
        mdp.poll();
        sleep_ms(100);
    }
    mdp.getDeviceInfo();

    mdp.setAllLEDs(0);
    sendLeds_monome();

    uint32_t last_refresh = to_ms_since_boot(get_absolute_time());

    while (true) {
        tud_task();
        mdp.poll();

        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_refresh >= 16) {
            trellis.read();
            sendLeds_monome();
            last_refresh = now;
        }
        tud_cdc_write_flush();
    }
}
