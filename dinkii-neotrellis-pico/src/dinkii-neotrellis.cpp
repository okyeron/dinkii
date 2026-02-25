/***********************************************************
 *  DIY monome compatible grid w/ Adafruit NeoTrellis
 *  for RP2040 Pi Pico
 *
 *  This code makes the Adafruit Neotrellis boards into a Monome compatible grid
 * via monome's mext protocol
 *  ----> https://www.adafruit.com/product/3954
 *
 *  Code here is for a 16x8 grid, but can be modified for 4x8, 8x8, or 16x16
 * (untested on larger grid arrays)
 *
 *  Many thanks to:
 *  scanner_darkly <https://github.com/scanner-darkly>,
 *  TheKitty <https://github.com/TheKitty>,
 *  Szymon Kaliski <https://github.com/szymonkaliski>,
 *  John Park, Todbot, Juanma, Gerald Stevens, and others
 *
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "MonomeSerialDevice.h"
#include "Adafruit_seesaw/Adafruit_NeoTrellis.h"
#include "config.h"

#include "bsp/board.h"
#include "tusb.h"

struct ElapsedMillis {
  uint32_t _start;
  ElapsedMillis() : _start(to_ms_since_boot(get_absolute_time())) {}
  operator uint32_t() const { return to_ms_since_boot(get_absolute_time()) - _start; }
  ElapsedMillis& operator=(uint32_t v) { _start = to_ms_since_boot(get_absolute_time()) - v; return *this; }
};

ElapsedMillis monomeRefresh;

uint8_t g_monome_mode = 1; // 0 is midi, 1 is monome


// Monome class setup
MonomeSerialDevice mdp;
int prevLedBuffer[mdp.MAXLEDCOUNT];
bool isInited = false;
bool isDirty = false;

// NeoTrellis setup
#if GRIDCOUNT == SIXTEEN
// 1X1 TEST -- USES 8x8 address set
Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    {Adafruit_NeoTrellis(addrRowOne[1])}};
#endif

#if GRIDCOUNT == SIXTYFOUR
// 8x8
Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    {Adafruit_NeoTrellis(addrRowOne[0]),
     Adafruit_NeoTrellis(addrRowOne[1])},
    {Adafruit_NeoTrellis(addrRowTwo[0]),
     Adafruit_NeoTrellis(addrRowTwo[1])}};
#endif

#if GRIDCOUNT == ONETWENTEIGHT
// 16x8
Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    {Adafruit_NeoTrellis(addrRowOne[0]),
     Adafruit_NeoTrellis(addrRowOne[1]),
     Adafruit_NeoTrellis(addrRowOne[2]),
     Adafruit_NeoTrellis(addrRowOne[3])}, // top row
    {Adafruit_NeoTrellis(addrRowTwo[0]),
     Adafruit_NeoTrellis(addrRowTwo[1]),
     Adafruit_NeoTrellis(addrRowTwo[2]),
     Adafruit_NeoTrellis(addrRowTwo[3])} // bottom row
};
#endif

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)trellis_array,
                              NUM_ROWS / 4, NUM_COLS / 4);

// ***************************************************************************
// **                                HELPERS                                **
// ***************************************************************************

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}

// ***************************************************************************
// **                                SEND LEDS                              **
// ***************************************************************************

void sendLeds() {
  uint8_t value, prevValue = 0;
  uint32_t hexColor;
  bool isDirty = false;

  for (int i = 0; i < NUM_ROWS * NUM_COLS; i++) {
    value = mdp.leds[i];
    prevValue = prevLedBuffer[i];
    uint8_t gvalue = gammaTable[value] * gammaAdj;

    if (value != prevValue) {
      // hexColor = (((R * value) >> 4) << 16) + (((G * value) >> 4) << 8) + ((B
      // * value) >> 4);
      hexColor = (((gvalue * R) / 256) << 16) + (((gvalue * G) / 256) << 8) +
                 (((gvalue * B) / 256) << 0);
      trellis.setPixelColor(i, hexColor);

      prevLedBuffer[i] = value;
      isDirty = true;
    }
  }
  if (isDirty) {
    trellis.show();
  }
}

// ***************************************************************************
// **                          FUNCTIONS FOR TRELLIS                        **
// ***************************************************************************

// define a callback for key presses
TrellisCallback keyCallback(keyEvent evt) {
  uint8_t x = evt.bit.NUM % NUM_COLS;
  uint8_t y = evt.bit.NUM / NUM_COLS;

  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    //     Serial.println(" pressed ");
    mdp.sendGridKey(x, y, 1);
#if TEST
    trellis.setPixelColor(
        evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, NUM_ROWS, 0, 255))); // on rising
    trellis.show();
#endif
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    //     Serial.println(" released ");
    mdp.sendGridKey(x, y, 0);
#if TEST
    trellis.setPixelColor(evt.bit.NUM, 0); // off falling
    trellis.show();
#endif
  }
  sendLeds();
  // trellis.show();
  return 0;
}


char serialNumberBuf[64];
// ***************************************************************************
// **                                 SETUP                                 **
// ***************************************************************************

void SetupBoard()
{
  board_init(); // TinyUSB board init

  // Board LED init
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_put(LED_PIN, 1);
  tusb_init();
}

void trellisSetup() {
  uint8_t x, y;

  mdp.isMonome = true;
  mdp.deviceID = deviceID;
  mdp.setupAsGrid(NUM_ROWS, NUM_COLS);
  monomeRefresh = 0;
  isInited = true;

  int var = 0;
  while (var < 8) {
    tud_task();
    mdp.poll();
    var++;
    sleep_ms(100);
  }
  mdp.getDeviceInfo();

  trellis.begin();

  // key callback
  for (x = 0; x < NUM_COLS; x++) {
    for (y = 0; y < NUM_ROWS; y++) {
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
      trellis.registerCallback(x, y, keyCallback);
    }
  }

  // set overall brightness for all pixels
  for (x = 0; x < NUM_COLS / 4; x++) {
    for (y = 0; y < NUM_ROWS / 4; y++) {
      trellis_array[y][x].pixels.setBrightness(BRIGHTNESS);
    }
  }

  // clear grid leds
  mdp.setAllLEDs(0);
  sendLeds();

  // blink one led to show it's started up
  trellis.setPixelColor(0, 0xFFFFFF);
  trellis.show();
  sleep_ms(100);
  trellis.setPixelColor(0, 0x000000);
  trellis.show();

#if TEST
  /* the array can be addressed as x,y or with the key number */
  for (int i = 0; i < NUM_LEDS; i++) {
    trellis.setPixelColor(i, 0xFFFFFF); 
    trellis.show();
    sleep_ms(50);
  }
#endif
}

// ***************************************************************************
// **                            MAIN LOOP                                  **
// ***************************************************************************

int main() {

  // stdio_init_all();

  SetupBoard();

  trellisSetup();

  while (true)
  {
    tud_task();

    mdp.poll(); // process incoming serial from Monomes

    // refresh every 16ms or so
    if (isInited && monomeRefresh > 16) {
      // if (mdp.tiltState[0]) {
      //   mdp.sendTiltEvent(0, axisbytes[0], axisbytes[1], axisbytes[2],
      //                     axisbytes[3], axisbytes[4], axisbytes[5]);
      // }
      trellis.read();
      sendLeds();
      monomeRefresh = 0;
    }

    //     // Send tilt data every 100ms
    //     if (currentMillis - lastTiltCheck >= 100) {
    //         lastTiltCheck = currentMillis;
    //         sendTiltData();
    //     }
    tud_cdc_write_flush();
  }
  return 0;
}

// void sendTiltData() {
//     sensors_event_t a, g, temp;
//     mpu.getEvent(&a, &g, &temp);
//
//     // Scale gyro data to 16-bit integer range
//     int16_t x = (int16_t)(g.gyro.x * 1000);
//     int16_t y = (int16_t)(g.gyro.y * 1000);
//     int16_t z = (int16_t)(g.gyro.z * 1000);
//
//     mdp.sendTiltEvent(0, x, y, z);
// }