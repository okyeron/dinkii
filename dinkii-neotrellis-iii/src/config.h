#include <stdlib.h>
#include "pico/stdlib.h"
#include <stdint.h>

#define TEST 0 // SET TO 1 for testing

#define SIXTEEN 1
#define SIXTYFOUR 2
#define ONETWENTYEIGHT 3
#define TWOFIFTYSIX 4

// Which Grid - SIXTEEN, SIXTYFOUR, ONETWENTYEIGHT, TWOFIFTYSIX
#ifndef GRIDCOUNT
#define GRIDCOUNT SIXTYFOUR
#endif

#if GRIDCOUNT == SIXTEEN
#define NUM_ROWS 4 // down - rows
#define NUM_COLS 4 // across - columns
static const uint8_t addrRowOne[1] = {0x2F}; 
#endif
#if GRIDCOUNT == SIXTYFOUR
#define NUM_ROWS 8 // down - rows
#define NUM_COLS 8 // across - columns
static const uint8_t addrRowOne[2] = {0x2F,0x2E}; 
static const uint8_t addrRowTwo[2] = {0x3E,0x36}; 
#endif
#if GRIDCOUNT == ONETWENTYEIGHT
#define NUM_ROWS 8  // down - rows
#define NUM_COLS 16 // across - columns
const uint8_t addrRowOne[4] = {0x32,0x30,0x2F,0x2E}; 
const uint8_t addrRowTwo[4] = {0x33,0x31,0x3E,0x36}; 
#endif
#if GRIDCOUNT == TWOFIFTYSIX
#define NUM_ROWS 16 // down - rows
#define NUM_COLS 16 // across - columns
// for 16x16
// static const uint8_t addrRowOne[4] = {0x32,0x30,0x2F,0x2E}; 
// static const uint8_t addrRowTwo[4] = {0x33,0x31,0x3E,0x36}; 
// static const uint8_t addrRowThree[4] = {0x3c,0x40,0x38,0x34}; 
// static const uint8_t addrRowFour[4] = {0x46,0x4a,0x42,0x3a}; 

#endif

#define NUM_LEDS NUM_ROWS *NUM_COLS

#define INT_PIN 9
// #define LED_PIN 13 // teensy LED used to show boot info
#define LED_PIN 16  // dinkii LED1
#define LED_PIN2 18 // dinkii LED2

// This assumes you are using a USB breakout board to route power to the board
// If you are plugging directly into the controller, you will need to adjust
// this brightness to a much lower value
#define BRIGHTNESS                                                             \
  96 // overall grid brightness
     // may need adjustment for larger grids
     // use gammaTable and gammaAdj below to adjust levels

#define R 255
#define G 255
#define B 255

// gamma table for 16 levels of brightness
static const uint8_t gammaTable[16] = {0,  2,  3,  6,  11, 18,  25,  32,
                                       41, 59, 70, 80, 92, 103, 115, 127};
static const uint8_t gammaAdj = 1; // from 1 to 2

// set your monome device name here
static const char* deviceID = "monome";
static const char* serialNum = "m4216126";

// DEVICE INFO FOR TinyUSB
// static: prevents multiple-definition errors when included from multiple TUs
static char mfgstr[32] = "monome";
static char prodstr[32] = "grid";
static char serialstr[32] = "m4216126";

#define mapRange(s,a1,a2,b1,b2) (b1 + (s-a1)*(b2-b1)/(a2-a1))

#define DEBOUNCETIME 10
class debounce_t
{
public: 
    int timeon = 0;
    int timeoff = DEBOUNCETIME;
} ;

enum
{
    UNCERTAIN,
    PRESSED,
    RELEASED,
    DOWN,
    UP
};
void trellisUpdate();
uint16_t GetKeyState(int idx);