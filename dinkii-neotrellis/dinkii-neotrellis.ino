/***********************************************************
 *  DIY monome compatible grid w/ Adafruit NeoTrellis
 *  for RP2040 Pi Pico
 *  
 *  This code makes the Adafruit Neotrellis boards into a Monome compatible grid via monome's mext protocol
 *  ----> https://www.adafruit.com/product/3954
 *  
 *  Code here is for a 16x8 grid, but can be modified for 4x8, 8x8, or 16x16 (untested on larger grid arrays)
 *  
 *  Many thanks to: 
 *  scanner_darkly <https://github.com/scanner-darkly>, 
 *  TheKitty <https://github.com/TheKitty>, 
 *  Szymon Kaliski <https://github.com/szymonkaliski>, 
 *  John Park, Todbot, Juanma, Gerald Stevens, and others
 *
*/

// SET TOOLS USB STACK TO TinyUSB

// RP2040 BOARDS REQUIRE Earle Philhower's Arduino core for RP2040 devices, arduino-pico
// See https://learn.adafruit.com/rp2040-arduino-with-the-earlephilhower-core/overview

// Be sure you have these libraries installed
//    Adafruit seesaw library 
//    elapsedMillis
//    Adafruit TinyUSB Library
//    Adafruit NeoPixel


#include "MonomeSerialDevice.h"
#include <Adafruit_NeoTrellis.h>

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <elapsedMillis.h>

#include "i2c_config.h" // look here to change settings for different boards

#include "config.h" // look here to change settings for different boards

#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL343.h>
Adafruit_ADXL343 accel = Adafruit_ADXL343(12345, &MYWIRE);



bool isInited = false;
elapsedMillis monomeRefresh;

// Monome class setup
MonomeSerialDevice mdp;

int prevLedBuffer[mdp.MAXLEDCOUNT]; 


// NeoTrellis setup
#if GRIDCOUNT == SIXTEEN
  // 1X1 TEST -- USES 8x8 address set
  Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    { Adafruit_NeoTrellis(addrRowOne[1], &MYWIRE) }
  };
#endif

#if GRIDCOUNT == SIXTYFOUR
  // 8x8
  Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    { Adafruit_NeoTrellis(addrRowOne[0], &MYWIRE), Adafruit_NeoTrellis(addrRowOne[1], &MYWIRE) },
    { Adafruit_NeoTrellis(addrRowTwo[0], &MYWIRE), Adafruit_NeoTrellis(addrRowTwo[1], &MYWIRE) }
  };
#endif

#if GRIDCOUNT == ONETWENTEIGHT
  // 16x8
  Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    { Adafruit_NeoTrellis(addrRowOne[0], &MYWIRE), Adafruit_NeoTrellis(addrRowOne[1], &MYWIRE), Adafruit_NeoTrellis(addrRowOne[2], &MYWIRE), Adafruit_NeoTrellis(addrRowOne[3], &MYWIRE)}, // top row
    { Adafruit_NeoTrellis(addrRowTwo[0], &MYWIRE), Adafruit_NeoTrellis(addrRowTwo[1], &MYWIRE), Adafruit_NeoTrellis(addrRowTwo[2], &MYWIRE), Adafruit_NeoTrellis(addrRowTwo[3], &MYWIRE) } // bottom row
  };
#endif



Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)trellis_array, NUM_ROWS / 4, NUM_COLS / 4);


// ***************************************************************************
// **                                HELPERS                                **
// ***************************************************************************

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}

// ***************************************************************************
// **                          FUNCTIONS FOR TRELLIS                        **   
// ***************************************************************************


//define a callback for key presses
TrellisCallback keyCallback(keyEvent evt){
  uint8_t x  = evt.bit.NUM % NUM_COLS;
  uint8_t y = evt.bit.NUM / NUM_COLS; 

  if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING){
//     Serial.println(" pressed ");
    mdp.sendGridKey(x, y, 1);
    #if TEST
      trellis.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, NUM_ROWS, 0, 255))); //on rising
      trellis.show();
    #endif
  }else if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING){
//     Serial.println(" released ");
    mdp.sendGridKey(x, y, 0);
    #if TEST
      trellis.setPixelColor(evt.bit.NUM, 0); //off falling
      trellis.show();
    #endif
  }
  sendLeds();
  // trellis.show();
  return 0;
}


String serialNumberOne;
String serialNumberTwo;
// ***************************************************************************
// **                                 SETUP                                 **
// ***************************************************************************

void setup(){
	uint8_t x, y;

	TinyUSBDevice.setManufacturerDescriptor(mfgstr);
	TinyUSBDevice.setProductDescriptor(prodstr);
	uint16_t tusb_serial[16];
  int validindices = TinyUSBDevice.getSerialDescriptor(tusb_serial);
  uint8_t serial_id[16] __attribute__((aligned(4)));
  uint8_t const serial_len = TinyUSB_Port_GetSerialNumber(serial_id);
  for (int i =0 ;i<8 ;i++) {
    serialNumberOne = String(serial_id[i], DEC); 
    serialNumberTwo.concat(serialNumberOne);
  }
  String tempSerial = serialNumberTwo.substring(9);
  tempSerial.setCharAt(0, 'm');
  TinyUSBDevice.setSerialDescriptor(tempSerial.c_str());

	Serial.begin(115200);

  // while( !TinyUSBDevice.mounted() ) delay(1);

	MYWIRE.setSDA(I2C_SDA);
	MYWIRE.setSCL(I2C_SCL);

  // Serial.println(I2C_SDA);
  // Serial.println(I2C_SCL);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

	mdp.isMonome = true;
	mdp.deviceID = deviceID;
	mdp.setupAsGrid(NUM_ROWS, NUM_COLS);
  	monomeRefresh = 0;
  	isInited = true;

	int var = 0;
	while (var < 8) {
		mdp.poll();
		var++;
		delay(100);
	}

	if (!trellis.begin()) {
		// Serial.println("trellis.begin() failed!");
		// Serial.println("check your addresses.");
		// Serial.println("reset to try again.");
    // Serial.println(serialNumberTwo);

		while(1);  // loop forever
	}
  
  // ADXL343 Accelerometer init
  /* Initialise the sensor */
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL343 ... check your connections */
    // Serial.println("No ADXL343 detected ... Check your wiring!");
    // while(1);
  }
  /* Set the range to whatever is appropriate for your project */
  accel.setRange(ADXL343_RANGE_16_G); // 2/4/8/16 _G
  accel.setDataRate(ADXL343_DATARATE_100_HZ);


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
    delay(100);
    trellis.setPixelColor(0, 0x000000);
    trellis.show();

  #if TEST
  /* the array can be addressed as x,y or with the key number */
  for(int i=0; i<NUM_LEDS; i++){
      trellis.setPixelColor(i, Wheel(map(i, 0, NUM_COLS * NUM_ROWS, 0, 255))); //addressed with keynum
      trellis.show();
      delay(50);
  }
  #endif

}

// ***************************************************************************
// **                                SEND LEDS                              **
// ***************************************************************************

void sendLeds(){
  uint8_t value, prevValue = 0;
  uint32_t hexColor;
  bool isDirty = false;
  
  for(int i=0; i< NUM_ROWS * NUM_COLS; i++){
    value = mdp.leds[i];
    prevValue = prevLedBuffer[i];
    uint8_t gvalue = gammaTable[value] * gammaAdj;
    
    if (value != prevValue) {
      //hexColor = (((R * value) >> 4) << 16) + (((G * value) >> 4) << 8) + ((B * value) >> 4); 
      hexColor =  (((gvalue*R)/256) << 16) + (((gvalue*G)/256) << 8) + (((gvalue*B)/256) << 0);
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
// **                                 LOOP                                  **
// ***************************************************************************

void loop() {

    sensors_event_t event;
    accel.getEvent(&event);
    int16_t axis[3];
    axis[0] = (event.acceleration.x * 2) + 128;
    axis[1] = (event.acceleration.y * 2) + 128;
    axis[2] = (event.acceleration.z * 2) + 128;
    int8_t *axisbytes = (int8_t *)axis;
    // axisbytes[0],axisbytes[1]

    mdp.poll(); // process incoming serial from Monomes
 
    // refresh every 16ms or so
    if (isInited && monomeRefresh > 16) {
        if (mdp.tiltState[0]){
          mdp.sendTiltEvent(0,axisbytes[0],axisbytes[1],axisbytes[2],axisbytes[3],axisbytes[4],axisbytes[5]);
        }
        trellis.read();
        sendLeds();
        monomeRefresh = 0;
    }

//     // Send tilt data every 100ms
//     if (currentMillis - lastTiltCheck >= 100) {
//         lastTiltCheck = currentMillis;
//         sendTiltData();
//     }

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