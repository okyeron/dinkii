#include <stdint.h> 
#include <Arduino.h>

#define TEST 0    // SET TO 1 for testing

#define SIXTEEN 1
#define SIXTYFOUR 2
#define ONETWENTEIGHT 3
#define TWOFIFTYSIX 4

// Which Grid - SIXTEEN, SIXTYFOUR, ONETWENTEIGHT, TWOFIFTYSIX
#ifndef GRIDCOUNT
#define GRIDCOUNT ONETWENTEIGHT
#endif

#if GRIDCOUNT == SIXTEEN
  #define NUM_ROWS 4 // down - rows 
  #define NUM_COLS 4 // across - columns
#endif
#if GRIDCOUNT == SIXTYFOUR
  #define NUM_ROWS 8 // down - rows 
  #define NUM_COLS 8 // across - columns
#endif
#if GRIDCOUNT == ONETWENTEIGHT
  #define NUM_ROWS 8 // down - rows 
  #define NUM_COLS 16 // across - columns
#endif
#if GRIDCOUNT == TWOFIFTYSIX
  #define NUM_ROWS 16 // down - rows 
  #define NUM_COLS 16 // across - columns
#endif

#define NUM_LEDS NUM_ROWS*NUM_COLS

#define INT_PIN 9
// #define LED_PIN 13 // teensy LED used to show boot info
#define LED_PIN 16 // dinkii LED1
#define LED_PIN2 18 // dinkii LED2


// This assumes you are using a USB breakout board to route power to the board 
// If you are plugging directly into the controller, you will need to adjust this brightness to a much lower value
#define BRIGHTNESS 255  // overall grid brightness 
                        // may need adjustment for larger grids
                        // use gammaTable and gammaAdj below to adjust levels

#define R 255
#define G 255
#define B 255

// gamma table for 16 levels of brightness
const uint8_t gammaTable[16] = { 0,  2,  3,  6,  11, 18, 25, 32, 41, 59, 70, 80, 92, 103, 115, 127}; 
const uint8_t gammaAdj = 1; // from 1 to 2

// set your monome device name here
String deviceID = "monome";
String serialNum = "m4216126";

// DEVICE INFO FOR TinyUSB
char mfgstr[32] = "monome";
char prodstr[32] = "monome";
char serialstr[32] = "m4216126";
