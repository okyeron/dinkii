# dink-ii for NeoTrellis/MechaTrellis

dink-ii is an RP2040 based microcontroller designed for plug-n-play use with STEMMA-QT compatible i2c devices.

IMPORTANT - by default dink-ii sends 5v over the STEMMA-QT connector. If you need 3.3v for your device, see the jumper section below. (Note - NeoTrellis and MechaTrellis want 5v)

## Hardware

### NeoTrellis

Requires a JST-PH to JST-SH (same side) connector cable.

### MechaTrellis

Requires a JST-SH to JST-SH (opposite side) connector cable.

### 5v jumper

NeoTrellis and MechaTrellis use 5v so the STEMMA-QT connector defaults to 5v. If you need  3.3v instead for some other project, there is a 3-way jumper on the bottom of the board. For 3.3v, cut the trace between the center pad and the 5v pad, and then re-solder a bridge from the center pad to the 3.3v pad.

dink-ii boards ship with monome grid firmware installed in test mode (see board testing below)


## Firmware

### How to program/flash the board:

> To enter the bootloader, hold down the BOOTSEL button, and while continuing to hold it (don't let go!), press and release the reset button. Continue to hold the BOOTSEL button until the RPI-RP2 drive appears. Alternately you can hold the BOOTSEL button down while you plug the USB cable into your computer.

> Once the RPI-RP2 drive shows up, your board is in bootloader mode. Then make sure you are no longer holding down any buttons (RST or BOOTSEL button).

> Drag the appropriate UF2 file to the RPI-RP2 drive. The device will reset and should then function as expected.

For general programming using Arduino see the [Adafruit Guide for RP2040](https://learn.adafruit.com/adafruit-feather-rp2040-pico/arduino-ide-setup)



### Monome grid firmware

If re-programming the grid software for NeoTrellis/MechaTrellis, be aware of the following section of `dinkii-neotrellis.ino`

The idea here is that you can easily set the size of the grid and also enable a test mode.

For an 8x8 gird, use a `GRIDCOUNT` value of `SIXTYFOUR`, for a 16x8 grid, use `ONETWENTEIGHT`, etc.

```
#define TEST 0    // SET TO 1 for testing

#define SIXTEEN 1
#define SIXTYFOUR 2
#define ONETWENTEIGHT 3
#define TWOFIFTYSIX 4

// Which Grid - SIXTEEN, SIXTYFOUR, ONETWENTEIGHT, TWOFIFTYSIX
#ifndef GRIDCOUNT
#define GRIDCOUNT ONETWENTEIGHT
#endif

```


### i2c_config

See the `i2c_config.h` file to change your board addresses based on your grid size.
You will need to uncomment/comment the appropriate lines here for your grid size.

```
// SET YOUR ADDRESSES 
// 8x8
// const uint8_t addrRowOne[2] = {0x2F,0x2E}; 
// const uint8_t addrRowTwo[2] = {0x3E,0x36}; 

// 16x8
// neotrellis standard layout
const uint8_t addrRowOne[4] = {0x32,0x30,0x2F,0x2E}; 
const uint8_t addrRowTwo[4] = {0x33,0x31,0x3E,0x36}; 
```


### Board testing 

If you want to test the LEDs on a single NeoTrellis/MechaTrellis board - which is advisable after purchase -  set the code above to the following

```
#define TEST 1    // SET TO 1 for testing

#define SIXTEEN 1
#define SIXTYFOUR 2
#define ONETWENTEIGHT 3
#define TWOFIFTYSIX 4

// Which Grid - SIXTEEN, SIXTYFOUR, ONETWENTEIGHT, TWOFIFTYSIX
#ifndef GRIDCOUNT
#define GRIDCOUNT SIXTEEN
#endif
```
