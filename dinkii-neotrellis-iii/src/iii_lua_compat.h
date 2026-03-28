#pragma once

/*
 * Compatibility shim for lib/iii/vm.c, which targets Lua 5.5.
 * We build against Lua 5.4.x, which lacks two APIs:
 *   - luaL_makeseed(L) : generates a PRNG seed
 *   - lua_newstate(f, ud, seed) : 3-arg form
 *
 * This header is force-included for lib/iii/vm.c only via CMake:
 *   set_source_files_properties(lib/iii/vm.c PROPERTIES
 *       COMPILE_OPTIONS "-include;${CMAKE_SOURCE_DIR}/src/iii_lua_compat.h")
 */

#include "lua.h"
#include "lauxlib.h"

/* luaL_makeseed: returns a seed value for the Lua PRNG. */
static inline unsigned lua_compat_makeseed(lua_State *L) {
    (void)L;
    return 12345u;
}
#define luaL_makeseed lua_compat_makeseed

/*
 * lua_newstate: Lua 5.5 adds a third 'seed' arg; 5.4 takes only f and ud.
 * Parenthesised (lua_newstate) invokes the real function, bypassing this macro.
 */
static inline lua_State *lua_newstate_compat(lua_Alloc f, void *ud,
                                             unsigned seed) {
    (void)seed;
    return (lua_newstate)(f, ud);
}
#define lua_newstate lua_newstate_compat
