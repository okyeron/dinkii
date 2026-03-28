/*
 * device_lua.c — Lua bindings for the grid device
 *
 * This file MUST be compiled without LUA_USE_C89 so that lua_Integer
 * resolves to long long (8 bytes), matching the Lua library.  With
 * LUA_USE_C89 active lua_Integer becomes long (4 bytes), which causes an
 * ARM AAPCS calling-convention mismatch: the caller passes a long long in
 * r2:r3 (8-byte aligned pair) while the callee reads a long from r1 —
 * producing garbage values in every lua_pushinteger / lua_tointeger call.
 */
#ifdef LUA_USE_C89
#undef LUA_USE_C89
#endif

#include "lua.h"
#include "lauxlib.h"

#include "device_ext.h"
#include "vm.h"
#include "serial.h"

// ---------------------------------------------------------------------------
// vm_handle_grid_key — call Lua event_grid(x, y, z)
// Called from device_task() in device.cpp.
// ---------------------------------------------------------------------------
void vm_handle_grid_key(uint8_t x, uint8_t y, uint8_t z) {
    if (L == NULL) return;
    lua_getglobal(L, "event_grid");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_getglobal(L, "grid");  // alias
    }
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    lua_pushinteger(L, x + 1);  // 1-based
    lua_pushinteger(L, y + 1);  // 1-based
    lua_pushinteger(L, z);
    l_report(L, docall(L, 3, 0));
}

// ---------------------------------------------------------------------------
// Lua grid bindings
// ---------------------------------------------------------------------------

static int l_grid_led(lua_State *l) {
    int x   = (int)lua_tointeger(l, 1) - 1;
    int y   = (int)lua_tointeger(l, 2) - 1;
    int z   = (int)lua_tointeger(l, 3);
    int rel = lua_toboolean(l, 4);
    device_led_set(x, y, z, rel);
    return 0;
}

static int l_grid_led_get(lua_State *l) {
    int x = (int)lua_tointeger(l, 1) - 1;
    int y = (int)lua_tointeger(l, 2) - 1;
    lua_pushinteger(l, (lua_Integer)device_led_get(x, y));
    return 1;
}

static int l_grid_led_all(lua_State *l) {
    int z   = (int)lua_tointeger(l, 1);
    int rel = lua_toboolean(l, 2);
    device_led_all(z, rel);
    return 0;
}

static int l_grid_intensity(lua_State *l) {
    int z = (int)lua_tointeger(l, 1);
    device_intensity(z);
    return 0;
}

static int l_grid_color(lua_State *l) {
    int r = (int)lua_tointeger(l, 1);
    int g = (int)lua_tointeger(l, 2);
    int b = (int)lua_tointeger(l, 3);
    device_color_set(r, g, b);
    return 0;
}

static int l_grid_refresh(lua_State *l) {
    (void)l;
    device_mark_dirty();
    return 0;
}

static int l_grid_size_x(lua_State *l) {
    (void)l;
    lua_pushinteger(l, (lua_Integer)device_cols());
    return 1;
}

static int l_grid_size_y(lua_State *l) {
    (void)l;
    lua_pushinteger(l, (lua_Integer)device_rows());
    return 1;
}

static const luaL_Reg device_lib[] = {
    {"grid_led",       l_grid_led},
    {"grid_led_get",   l_grid_led_get},
    {"grid_led_all",   l_grid_led_all},
    {"grid_intensity", l_grid_intensity},
    {"grid_color",     l_grid_color},
    {"grid_refresh",   l_grid_refresh},
    {"grid_size_x",    l_grid_size_x},
    {"grid_size_y",    l_grid_size_y},
    {NULL, NULL}
};

const luaL_Reg *get_device_lib(void) {
    return device_lib;
}
