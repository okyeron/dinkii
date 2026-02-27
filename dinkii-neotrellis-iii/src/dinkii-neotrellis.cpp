/***********************************************************
 *  DIY monome compatible grid w/ Adafruit NeoTrellis
 *  for RP2040 Pi Pico
 *
 *  Supports two modes stored in flash:
 *    mode 0 (iii)    - Lua scripting via iii
 *    mode 1 (monome) - standard monome serial protocol
 *
 *  Hold any grid key during boot to toggle between modes.
 *  In mode 1→0 transition, continue holding (~2.5 s) to skip init.lua.
 *
 */

#include <stdint.h>
#include "pico/stdlib.h"
#include "config.h"
#include "bsp/board.h"
#include "tusb.h"

extern "C" {
#include "flash.h"
#include "iii.h"
#include "device.h"
}

// USB descriptor selector — read by tud_descriptor_device_cb() in usb_descriptors.cpp
//   0 = DINKII descriptor (CDC+MIDI, used for iii/REPL mode)
//   1 = MONOME descriptor  (CDC+MIDI, monome VID/PID)
uint8_t g_monome_mode = 1;

// Current device mode — also read by device.cpp for dispatch decisions
//   0 = iii (Lua scripting)
//   1 = monome serial protocol
uint8_t mode = 1;

// Defined in device.cpp — blocking monome protocol loop
extern "C" void device_monome_loop();

// ***************************************************************************
// **                                 SETUP                                 **
// ***************************************************************************

void SetupBoard() {
    board_init();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    tusb_init();
}

// ***************************************************************************
// **                                  MAIN                                 **
// ***************************************************************************

int main() {
    // flash_init() must be the very first call — it locates the FS partition
    // and reads/writes the mode byte.
    flash_init();

    bool run_script = true;
    bool reset = check_device_key(); // true if a grid key is held at boot
    mode = flash_read_mode();

    if (reset) {
        if (mode == 1) {
            // monome → iii
            flash_write_mode(0);
            mode = 0;
            // Keep holding to skip init.lua (useful for recovery)
            uint16_t count = 0;
            while (check_device_key() && count < 256) {
                sleep_ms(10);
                count++;
            }
            if (count >= 256) run_script = false;
        } else {
            // iii → monome
            flash_write_mode(1);
            mode = 1;
        }
    }

    // Set USB descriptor before tusb_init() so enumeration uses correct IDs:
    //   mode 0 (iii)    → DINKII USB  (needs CDC for REPL)
    //   mode 1 (monome) → MONOME USB  (monome VID/PID for host compatibility)
    g_monome_mode = mode;

    SetupBoard();
    device_init(); // NeoTrellis hardware init (shared by both modes)

    if (mode == 0) {
        iii_loop(run_script); // never returns; calls device_task() internally
    } else {
        device_monome_loop(); // never returns
    }

    return 0;
}