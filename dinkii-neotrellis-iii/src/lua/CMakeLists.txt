set(lua_sources
       ${CMAKE_CURRENT_LIST_DIR}/lapi.c
       ${CMAKE_CURRENT_LIST_DIR}/lauxlib.c
       ${CMAKE_CURRENT_LIST_DIR}/lbaselib.c
       ${CMAKE_CURRENT_LIST_DIR}/lcode.c
       ${CMAKE_CURRENT_LIST_DIR}/lcorolib.c
       ${CMAKE_CURRENT_LIST_DIR}/lctype.c
       ${CMAKE_CURRENT_LIST_DIR}/ldblib.c
       ${CMAKE_CURRENT_LIST_DIR}/ldebug.c
       ${CMAKE_CURRENT_LIST_DIR}/ldo.c
       ${CMAKE_CURRENT_LIST_DIR}/ldump.c
       ${CMAKE_CURRENT_LIST_DIR}/lfunc.c
       ${CMAKE_CURRENT_LIST_DIR}/lgc.c
       ${CMAKE_CURRENT_LIST_DIR}/linit.c
       ${CMAKE_CURRENT_LIST_DIR}/liolib.c
       ${CMAKE_CURRENT_LIST_DIR}/llex.c
       ${CMAKE_CURRENT_LIST_DIR}/lmathlib.c
       ${CMAKE_CURRENT_LIST_DIR}/lmem.c
       ${CMAKE_CURRENT_LIST_DIR}/loadlib.c
       ${CMAKE_CURRENT_LIST_DIR}/lobject.c
       ${CMAKE_CURRENT_LIST_DIR}/lopcodes.c
       ${CMAKE_CURRENT_LIST_DIR}/loslib.c
       ${CMAKE_CURRENT_LIST_DIR}/lparser.c
       ${CMAKE_CURRENT_LIST_DIR}/lstate.c
       ${CMAKE_CURRENT_LIST_DIR}/lstring.c
       ${CMAKE_CURRENT_LIST_DIR}/lstrlib.c
       ${CMAKE_CURRENT_LIST_DIR}/ltable.c
       ${CMAKE_CURRENT_LIST_DIR}/ltablib.c
       ${CMAKE_CURRENT_LIST_DIR}/ltm.c
       ${CMAKE_CURRENT_LIST_DIR}/lundump.c
       ${CMAKE_CURRENT_LIST_DIR}/lutf8lib.c
       ${CMAKE_CURRENT_LIST_DIR}/lvm.c
       ${CMAKE_CURRENT_LIST_DIR}/lzio.c
)

set(lua_private_headers
       ${CMAKE_CURRENT_LIST_DIR}/lapi.h
       ${CMAKE_CURRENT_LIST_DIR}/lauxlib.h
       ${CMAKE_CURRENT_LIST_DIR}/lcode.h
       ${CMAKE_CURRENT_LIST_DIR}/lctype.h
       ${CMAKE_CURRENT_LIST_DIR}/ldebug.h
       ${CMAKE_CURRENT_LIST_DIR}/ldo.h
       ${CMAKE_CURRENT_LIST_DIR}/lfunc.h
       ${CMAKE_CURRENT_LIST_DIR}/lgc.h
       ${CMAKE_CURRENT_LIST_DIR}/ljumptab.h
       ${CMAKE_CURRENT_LIST_DIR}/llex.h
       ${CMAKE_CURRENT_LIST_DIR}/llimits.h
       ${CMAKE_CURRENT_LIST_DIR}/lmem.h
       ${CMAKE_CURRENT_LIST_DIR}/lobject.h
       ${CMAKE_CURRENT_LIST_DIR}/lopcodes.h
       ${CMAKE_CURRENT_LIST_DIR}/lopnames.h
       ${CMAKE_CURRENT_LIST_DIR}/lparser.h
       ${CMAKE_CURRENT_LIST_DIR}/lprefix.h
       ${CMAKE_CURRENT_LIST_DIR}/lstate.h
       ${CMAKE_CURRENT_LIST_DIR}/lstring.h
       ${CMAKE_CURRENT_LIST_DIR}/ltable.h
       ${CMAKE_CURRENT_LIST_DIR}/ltm.h
       ${CMAKE_CURRENT_LIST_DIR}/lua.h
       ${CMAKE_CURRENT_LIST_DIR}/lualib.h
       ${CMAKE_CURRENT_LIST_DIR}/lundump.h
       ${CMAKE_CURRENT_LIST_DIR}/lvm.h
       ${CMAKE_CURRENT_LIST_DIR}/lzio.h
)

set(lua_public_headers
       ${CMAKE_CURRENT_LIST_DIR}/lauxlib.h
       ${CMAKE_CURRENT_LIST_DIR}/lua.h
       ${CMAKE_CURRENT_LIST_DIR}/luaconf.h
       ${CMAKE_CURRENT_LIST_DIR}/lualib.h
)


add_library(lua STATIC)
target_sources(lua PRIVATE ${lua_sources})
target_sources(lua PRIVATE ${lua_private_headers} ${lua_public_headers})
