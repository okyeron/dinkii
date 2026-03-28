#ifndef DINKII_DEVICE_EXT_H
#define DINKII_DEVICE_EXT_H

#include <stdint.h>

// ---------------------------------------------------------------------------
// LED C API — implemented in device.cpp (C++), called from device_lua.c (C).
// Uses int throughout to avoid bool/int8_t header dependencies.
// rel: 0 = absolute, 1 = relative
// ---------------------------------------------------------------------------
extern void device_led_set(int x, int y, int z, int rel);
extern int  device_led_get(int x, int y);
extern void device_led_all(int z, int rel);
extern void device_intensity(int z);
extern void device_mark_dirty(void);
extern int  device_cols(void);
extern int  device_rows(void);
extern void device_color_set(int r, int g, int b);

// Defined in device_lua.c; called from device_task() in device.cpp.
extern void vm_handle_grid_key(uint8_t x, uint8_t y, uint8_t z);

#endif // DINKII_DEVICE_EXT_H
