/***********************************************************
 *  DIY monome compatible grid w/ Adafruit NeoTrellis
 *  for RP2040 Pi Pico
 *
 *  Supports two modes stored in flash:
 *    mode 0 (iii)    - Lua scripting via iii
 *    mode 1 (monome) - standard monome serial protocol
 *
 *
 */

#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "config.h"
#include "bsp/board.h"
#include "tusb.h"

extern "C" {
#include "flash.h"
#include "iii.h"
#include "device.h"
}

// USB descriptor selector — read by tud_descriptor_device_cb() in usb_descriptors.cpp
//   0 = iii descriptor (CDC+MIDI, used for iii/REPL mode)
//   1 = MONOME descriptor  (CDC SERIAL, monome VID/PID)
uint8_t g_monome_mode = 0;

// Current device mode — also read by device.cpp
//   0 = iii (Lua scripting)
//   1 = monome serial protocol
uint8_t mode = 0;

// Defined in device.cpp — blocking monome protocol loop
extern "C" void device_monome_loop();

// Core 1: lockout victim for safe flash operations from core 0.
// lfs_erase/lfs_prog in fs.c call multicore_lockout_start_blocking(),
// which requires the other core to have registered a lockout handler.
static void core1_entry() {
    multicore_lockout_victim_init();
    multicore_fifo_push_blocking(1); // signal core 0 that handler is ready
    while (true) tight_loop_contents();
}

// ***************************************************************************
// **                                 SETUP                                 **
// ***************************************************************************

void SetupBoard() {
    board_init();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);
    gpio_init(LED_PIN2);
    gpio_set_dir(LED_PIN2, GPIO_OUT);
    // gpio_put(LED_PIN2, 1);
}

// ***************************************************************************
// **                                  MAIN                                 **
// ***************************************************************************

int main() {
    // flash_init() must be the very first call — it locates the FS partition
    // and reads/writes the mode byte.
    flash_init();

    bool run_script = true;

    SetupBoard();
    device_init(); // NeoTrellis hardware init and mode_check();
 
    // reset USB descriptor before USB init
    g_monome_mode = mode;

    tud_init(BOARD_TUD_RHPORT);

    // Give USB time to stabilize
    sleep_ms(500);
    for (int i = 0; i < 500; i++) {
        tud_task();
        sleep_ms(1);
    }

    // Start core 1 as a multicore lockout victim before any LFS flash
    // operations (lfs_format, lfs_prog, lfs_erase). Core 1 does nothing
    // else; we wait for it to signal that its lockout handler is registered.
    multicore_launch_core1(core1_entry);
    multicore_fifo_pop_blocking(); // wait until core 1 handler is ready

    if (mode == 0) {
        gpio_put(LED_PIN2, 1);  // light LED2  when in iii mode
        iii_loop(run_script); // never returns; calls device_task() internally
    } else {
        gpio_put(LED_PIN, 1);   // light LED1  when in monome mode
        device_monome_loop(); // never returns
    }

    return 0;
}