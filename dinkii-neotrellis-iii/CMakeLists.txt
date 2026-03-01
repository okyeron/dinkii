# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if(EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

# Pull in the Pico SDK (must be before project())
include(pico_sdk_import.cmake)

project(dinkii-neotrellis-iii C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Export compile commands for IDE IntelliSense (clangd / VS Code)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico CACHE STRING "Board type")

# Initialize the Pico SDK
pico_sdk_init()

#------------------------------------------------------------------------
#--- embedded lua to c include

add_custom_command(
        OUTPUT
            ${CMAKE_SOURCE_DIR}/src/iii/resource/lib_lua.h
            ${CMAKE_SOURCE_DIR}/src/iii/resource/lib_lua.c
        COMMAND
            python3 ${CMAKE_SOURCE_DIR}/src/tools/embed.py ${CMAKE_SOURCE_DIR}/src/iii/lua/lib.lua ${CMAKE_SOURCE_DIR}/src/iii/resource
        DEPENDS
            ${CMAKE_SOURCE_DIR}/src/iii/lua/lib.lua
)

add_custom_target(embedded_resources DEPENDS ${CMAKE_SOURCE_DIR}/src/iii/lua/lib.lua)

# Main executable
add_executable(dinkii-neotrellis-iii
    src/dinkii-neotrellis.cpp
    src/device.cpp
    src/MonomeSerialDevice.cpp
    src/debug.cpp
    src/Adafruit_seesaw/Adafruit_NeoTrellis.cpp
    src/Adafruit_seesaw/Adafruit_seesaw.cpp
    src/Adafruit_seesaw/Adafruit_I2CDevice.cpp
    src/Adafruit_seesaw/seesaw_neopixel.cpp
    src/usb/usb_descriptors.cpp

    src/device_lua.c

    ${CMAKE_SOURCE_DIR}/src/iii/flash.c
    ${CMAKE_SOURCE_DIR}/src/iii/fs.c
    ${CMAKE_SOURCE_DIR}/src/iii/iii.c
    ${CMAKE_SOURCE_DIR}/src/iii/metro.c
    ${CMAKE_SOURCE_DIR}/src/iii/midi.c
    ${CMAKE_SOURCE_DIR}/src/iii/repl.c
    ${CMAKE_SOURCE_DIR}/src/iii/serial.c
    ${CMAKE_SOURCE_DIR}/src/iii/util.c
    ${CMAKE_SOURCE_DIR}/src/iii/vm.c
    ${CMAKE_SOURCE_DIR}/src/iii/resource/lib_lua.c
)

# Build defines — mirrors -DUSE_TINYUSB from platformio.ini
target_compile_definitions(dinkii-neotrellis-iii PRIVATE
    LUA_COMPAT_5_2
    LUA_USE_C89
    USE_TINYUSB=1
)
# # Force-include the Lua 5.4↔5.5 compatibility shim for iii's vm.c only.
# # vm.c calls lua_newstate(f,ud,seed) and luaL_makeseed(L) which are Lua 5.5 APIs.
# set_source_files_properties(src/iii/vm.c PROPERTIES
#     COMPILE_OPTIONS "-include;${CMAKE_CURRENT_SOURCE_DIR}/src/iii_lua_compat.h"
# )


# Include directories (headers live at the project root)
target_include_directories(dinkii-neotrellis-iii PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/usb
    ${CMAKE_SOURCE_DIR}/src/iii
    ${CMAKE_SOURCE_DIR}/src/lua

)
target_include_directories(dinkii-neotrellis-iii PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# add littleFS, built as separate static library
set(LFS_INCLUDES
    ${CMAKE_SOURCE_DIR}/src/littlefs
)
set(LFS_SOURCES
    ${CMAKE_SOURCE_DIR}/src/littlefs/lfs.c
	${CMAKE_SOURCE_DIR}/src/littlefs/lfs_util.c
)
add_library(lfs STATIC
    ${LFS_SOURCES}
)
target_include_directories(lfs PUBLIC
    ${LFS_INCLUDES}
)
target_compile_options(lfs PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-function)
target_include_directories(dinkii-neotrellis-iii PRIVATE ${LFS_INCLUDES})
target_link_libraries(dinkii-neotrellis-iii PRIVATE lfs)

add_subdirectory(src/lua)
target_link_libraries(dinkii-neotrellis-iii PRIVATE lua)

# pico-sdk link targets
target_link_libraries(dinkii-neotrellis-iii PUBLIC 
    pico_stdlib
    pico_multicore
    pico_unique_id
    pico_printf
    hardware_i2c
    hardware_gpio
    hardware_flash
    hardware_sync
    tinyusb_board
    tinyusb_device
)


pico_enable_stdio_usb(dinkii-neotrellis-iii 0)
pico_enable_stdio_uart(dinkii-neotrellis-iii 0)

# Generate UF2 and other binary output formats
pico_add_extra_outputs(dinkii-neotrellis-iii)
