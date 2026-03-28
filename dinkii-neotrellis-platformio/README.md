# dink-ii for NeoTrellis/MechaTrellis


## Firmware using PlatformIO or Arduino IDE



### How to program/flash the board:

> To enter the bootloader, hold down the BOOTSEL button, and while continuing to hold it (don't let go!), press and release the reset button. Continue to hold the BOOTSEL button until the RPI-RP2 drive appears. Alternately you can hold the BOOTSEL button down while you plug the USB cable into your computer.

> Once the RPI-RP2 drive shows up, your board is in bootloader mode. Then make sure you are no longer holding down any buttons (RST or BOOTSEL button).

> Drag the appropriate UF2 file to the RPI-RP2 drive. The device will reset and should then function as expected.

For general programming using Arduino see the [Adafruit Guide for RP2040](https://learn.adafruit.com/adafruit-feather-rp2040-pico/arduino-ide-setup)

### PlatformIO / VSCode  

The repo includes a PlatformIO configuration file and should work out of the box with VSCode.  

You may need to install some items if you've never used PlatformIO.  

On MacOS
Ensure Homebrew in installed. [Instructions](https://brew.sh/)    
Install PlatformIO CLI tools. [Detailed Instructions](https://platformio.org/install/cli)  
Install PlatformIO IDE VSCode extension [Instructions](https://platformio.org/platformio-ide)  



### Monome grid firmware

If re-programming the grid software for NeoTrellis/MechaTrellis, be aware of the following section of `dinkii-neotrellis.ino`

The idea here is that you can easily set the size of the grid and also enable a test mode.

For an 8x8 gird, use a `GRIDCOUNT` value of `SIXTYFOUR`, for a 16x8 grid, use `ONETWENTYEIGHT`, etc.

```
#define TEST 0    // SET TO 1 for testing

#define SIXTEEN 1
#define SIXTYFOUR 2
#define ONETWENTYEIGHT 3
#define TWOFIFTYSIX 4

// Which Grid - SIXTEEN, SIXTYFOUR, ONETWENTYEIGHT, TWOFIFTYSIX
#ifndef GRIDCOUNT
#define GRIDCOUNT ONETWENTYEIGHT
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

See board photos below for address jumpers on NeoTrellis and MechaTrellis.   

![<mechatrellis-addresses>](<../images/mechatrellis-addresses.png>)

![<neotrellis_addresses>](<../images/neotrellis_addresses.jpg>)


### Board testing 

If you want to test the LEDs on a single NeoTrellis/MechaTrellis board - which is advisable after purchase -  set the code above to the following

i2c_config should be set to the 8x8 addresses for this test mode.  

```
#define TEST 1    // SET TO 1 for testing

#define SIXTEEN 1
#define SIXTYFOUR 2
#define ONETWENTYEIGHT 3
#define TWOFIFTYSIX 4

// Which Grid - SIXTEEN, SIXTYFOUR, ONETWENTYEIGHT, TWOFIFTYSIX
#ifndef GRIDCOUNT
#define GRIDCOUNT SIXTEEN
#endif
```

