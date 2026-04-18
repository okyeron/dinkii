#include "MonomeSerialDevice.h"
// #include "debug.h"

MonomeSerialDevice::MonomeSerialDevice() {}

void MonomeSerialDevice::initialize() {
    active = false;
    isMonome = false;
    isGrid = true;
    rows = 0;
    columns = 0;
    encoders = 0;
    //clearQueue();
    clearAllLeds();
    arcDirty = false;
    gridDirty = false;
}

void MonomeSerialDevice::setupAsGrid(uint8_t _rows, uint8_t _columns) {
    initialize();
    active = true;
    isMonome = true;
    isGrid = true;
    rows = _rows;
    columns = _columns;
    gridDirty = true;
    // debugfln(INFO, "GRID rows: %d columns %d", rows, columns);
}

void MonomeSerialDevice::setupAsArc(uint8_t _encoders) {
    initialize();
    active = true;
    isMonome = true;
    isGrid = false;
    encoders = _encoders;
    arcDirty = true;
    // debugfln(INFO, "ARC encoders: %d", encoders);
}

void MonomeSerialDevice::getDeviceInfo() {
    //debugln(INFO, "MonomeSerialDevice::getDeviceInfo");
    tud_cdc_write_char(uint8_t(0));
    tud_cdc_n_write_flush(0);
 }

void MonomeSerialDevice::poll() {
    //while (isMonome && Serial.available()) { processSerial(); };
    while (tud_cdc_available()) {
      processSerial();
    }
    //Serial.println("processSerial");
}


void MonomeSerialDevice::setAllLEDs(uint8_t value) {
  for (int i = 0; i < MAXLEDCOUNT; i++) leds[i] = value;
}

void MonomeSerialDevice::setGridLed(uint8_t x, uint8_t y, uint8_t level) {
//    int index = x + (y * columns);   
//    if (index < MAXLEDCOUNT) leds[index] = level;

    if (x < columns && y < rows) {
      uint32_t index = y * columns + x;
      leds[index] = level;
    }
    //debugfln(INFO, "LED index: %d x %d y %d", index, x, y);
}
        
void MonomeSerialDevice::clearGridLed(uint8_t x, uint8_t y) {
    setGridLed(x, y, 0);
    //Serial.println("clearGridLed");
}

void MonomeSerialDevice::setArcLed(uint8_t enc, uint8_t led, uint8_t level) {
    int index = led + (enc << 6);
    if (index < MAXLEDCOUNT) leds[index] = level;
    //Serial.println("setArcLed");
}
        
void MonomeSerialDevice::clearArcLed(uint8_t enc, uint8_t led) {
    setArcLed(enc, led, 0);
    //Serial.println("clearArcLed");
}

void MonomeSerialDevice::clearAllLeds() {
    for (int i = 0; i < MAXLEDCOUNT; i++) leds[i] = 0;
    //Serial.println("clearAllLeds");
}

void MonomeSerialDevice::clearArcRing(uint8_t ring) {
    for (int i = ring << 6, upper = i + 64; i < upper; i++) leds[i] = 0;
    //Serial.println("clearArcRing");
}

void MonomeSerialDevice::refreshGrid() {
    gridDirty = true;
    //Serial.println("refreshGrid");
}

void MonomeSerialDevice::refreshArc() {
    arcDirty = true;
    //Serial.println("refreshArc");
}

void MonomeSerialDevice::refresh() {
/*
    uint8_t buf[35];
    int ind, led;

    if (gridDirty) {
        //Serial.println("gridDirty");
        buf[0] = 0x1A;
        buf[1] = 0;
        buf[2] = 0;

        ind = 3;
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x += 2) {
                led = (y << 4) + x;
                buf[ind++] = (leds[led] << 4) | leds[led + 1];
            }
        tud_cdc_write_char(buf, 35);
        
        ind = 3;
        buf[1] = 8;
        for (int y = 0; y < 8; y++)
            for (int x = 8; x < 16; x += 2) {
                led = (y << 4) + x;
                buf[ind++] = (leds[led] << 4) | leds[led + 1];
            }
        tud_cdc_write_char(buf, 35);
        
        ind = 3;
        buf[1] = 0;
        buf[2] = 8;
        for (int y = 8; y < 16; y++)
            for (int x = 0; x < 8; x += 2) {
                led = (y << 4) + x;
                buf[ind++] = (leds[led] << 4) | leds[led + 1];
            }
        tud_cdc_write_char(buf, 35);

        ind = 3;
        buf[1] = 8;
        for (int y = 8; y < 16; y++)
            for (int x = 8; x < 16; x += 2) {
                led = (y << 4) + x;
                buf[ind++] = (leds[led] << 4) | leds[led + 1];
            }
        tud_cdc_write_char(buf, 35);
        
        gridDirty = false;
    }

    if (arcDirty) {
        //Serial.print("arcDirty");
        buf[0] = 0x92;

        buf[1] = 0;
        ind = 2;
        for (led = 0; led < 64; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        tud_cdc_write_char(buf, 34);
        
        buf[1] = 1;
        ind = 2;
        for (led = 64; led < 128; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        tud_cdc_write_char(buf, 34);

        buf[1] = 2;
        ind = 2;
        for (led = 128; led < 192; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        tud_cdc_write_char(buf, 34);
        
        buf[1] = 3;
        ind = 2;
        for (led = 192; led < 256; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        tud_cdc_write_char(buf, 34);
        
        buf[1] = 4;
        ind = 2;
        for (led = 256; led < 320; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        tud_cdc_write_char(buf, 34);

        buf[1] = 5;
        ind = 2;
        for (led = 320; led < 384; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        tud_cdc_write_char(buf, 34);

        buf[1] = 6;
        ind = 2;
        for (led = 384; led < 448; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        tud_cdc_write_char(buf, 34);

        buf[1] = 7;
        ind = 2;
        for (led = 448; led < 512; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        tud_cdc_write_char(buf, 34);

        arcDirty = 0;
    }
*/
}

// Returns total packet length in bytes (inclusive of the command byte).
// Returns 0 for unknown commands.
uint8_t MonomeSerialDevice::get_cmd_length(uint8_t cmd) {
    switch (cmd) {
        case 0x00: return 1;   // system / query
        case 0x01: return 1;   // system / ID request
        case 0x02: return 33;  // system / write ID (1 + 32)
        case 0x03: return 1;   // system / report grid offset
        case 0x04: return 4;   // system / report ADDR
        case 0x05: return 1;   // system / get grid size
        case 0x06: return 3;   // system / set grid size
        case 0x07: return 1;   // I2C get addr
        case 0x08: return 3;   // I2C set addr
        case 0x0F: return 9;   // firmware version (1 + 8)
        case 0x10: return 3;   // led/set off
        case 0x11: return 3;   // led/set on
        case 0x12: return 1;   // led/all off
        case 0x13: return 1;   // led/all on
        case 0x14: return 11;  // led/map (1 + x + y + 8)
        case 0x15: return 4;   // led/row
        case 0x16: return 4;   // led/col
        case 0x17: return 2;   // led/intensity
        case 0x18: return 4;   // led/level/set
        case 0x19: return 2;   // led/level/all
        case 0x1A: return 35;  // led/level/map (1 + x + y + 32)
        case 0x1B: return 7;   // led/level/row (1 + x + y + 4)
        case 0x1C: return 7;   // led/level/col (1 + x + y + 4)
        case 0x20: return 3;   // key-grid / key up
        case 0x21: return 3;   // key-grid / key down
        case 0x50: return 3;   // enc / delta
        case 0x51: return 2;   // enc / key up
        case 0x52: return 2;   // enc / key down
        case 0x80: return 10;  // tilt active (1 + 9)
        case 0x81: return 9;   // tilt data (1 + 8)
        case 0x90: return 4;   // ring / set
        case 0x91: return 3;   // ring / all
        case 0x92: return 34;  // ring / map (1 + n + 32)
        case 0x93: return 5;   // ring / range
        default:   return 0;   // unknown
    }
}

void MonomeSerialDevice::processSerial() {
    uint8_t index, readX, readY, readN, readA;
    uint8_t dummy, gridNum, deviceAddress;
    uint8_t n, x, y, z, i;
    uint8_t intensity = 15;
    uint8_t gridKeyX;
    uint8_t gridKeyY;
    int8_t delta;
    uint8_t gridX    = columns;
    uint8_t gridY    = rows;
    uint8_t numQuads = columns/rows;

    // Read command byte, look up total packet length, then bulk-read the rest.
    // This prevents partial-read corruption when bytes arrive across USB packets.
    uint8_t identifierSent = tud_cdc_read_char();

    uint8_t len = get_cmd_length(identifierSent);
    if (len == 0) return;  // unknown command — discard

    uint8_t buf[34];  // max remaining bytes (35 byte cmd - 1 cmd byte)
    uint8_t buf_idx = 0;
    uint8_t remaining = len - 1;
    if (remaining > 0) {
        if (tud_cdc_n_available(0) < remaining) return;  // full packet not yet arrived
        tud_cdc_n_read(0, buf, remaining);
    }

    switch (identifierSent) {
        case 0x00:  // device information
        	// [null, "led-grid", "key-grid", "digital-out", "digital-in", "encoder", "analog-in", "analog-out", "tilt", "led-ring"]
            //Serial.println("0x00 system / query ----------------------");
            tud_cdc_write_char((uint8_t)0x00); // action: response, 0x00 = system
            tud_cdc_write_char((uint8_t)0x01); // section id, 1 = led-grid, 2 = key-grid, 5 = encoder/arc
            tud_cdc_write_char((uint8_t)numQuads);   // one Quad is 64 buttons

            tud_cdc_write_char((uint8_t)0x00); // send again with 2 = key-grid
            tud_cdc_write_char((uint8_t)0x02); // 
            tud_cdc_write_char((uint8_t)numQuads); 
            tud_cdc_n_write_flush(0);

            break;

        case 0x01:  // system / ID
            tud_cdc_write_char((uint8_t)0x01);        // action: response, 0x01
            for (i = 0; i < 32; i++) {          // has to be 32
                if (i < deviceID.length()) {
                  tud_cdc_write_char(deviceID[i]);
                } else {
                  tud_cdc_write_char((uint8_t)0x00);
                }
            }
            tud_cdc_n_write_flush(0);
            break;

        case 0x02:  // system / write ID
            deviceID.clear();
            for (int i = 0; i < 32; i++)
                deviceID += (char)buf[buf_idx++];
            tud_cdc_n_write_flush(0);
            break;

        case 0x03:  // system / report grid offset
            tud_cdc_write_char((uint8_t)0x02);
            tud_cdc_write_char((uint8_t)0x01);
            tud_cdc_write_char((uint8_t)0);     // x offset
            tud_cdc_write_char((uint8_t)0);     // y offset
            tud_cdc_n_write_flush(0);
            break;

        case 0x04:  // system / report ADDR
            gridNum     = buf[buf_idx++];
            readX       = buf[buf_idx++];
            readY       = buf[buf_idx++];
            break;

        case 0x05:  // system / get grid size
            tud_cdc_write_char((uint8_t)0x03);
            tud_cdc_write_char((uint8_t)gridX);
            tud_cdc_write_char((uint8_t)gridY);
            tud_cdc_n_write_flush(0);
            break;

        case 0x06:  // system / set grid size — ignored
            readX = buf[buf_idx++];
            readY = buf[buf_idx++];
            break;

        case 0x07:  // I2C get addr — ignored
            break;

        case 0x08:  // I2C set addr — ignored
            deviceAddress = buf[buf_idx++];
            dummy         = buf[buf_idx++];
            break;

        case 0x0F:  // firmware version
            buf_idx += 8;  // consume 8 bytes
            break;

      // 0x10-0x1F are for an LED Grid Control.  All bytes incoming, no responses back

        case 0x10:            // /prefix/led/set x y [0/1]  / led off
          readX = buf[buf_idx++];
          readY = buf[buf_idx++];
          setGridLed(readX, readY, 0);
          break;

        case 0x11:            // /prefix/led/set x y [0/1]  / led on
          readX = buf[buf_idx++];
          readY = buf[buf_idx++];
          setGridLed(readX, readY, 15);
          break;

        case 0x12:            //  /prefix/led/all [0/1]  / all off
          clearAllLeds();
          break;

        case 0x13:            //  /prefix/led/all [0/1]  / all on
          setAllLEDs(15);
          break;

        case 0x14:            // /prefix/led/map x y d[8]
          readX = buf[buf_idx++];
          while (readX > 16) { readX += 16; }
          readX &= 0xF8;
          readY = buf[buf_idx++];
          while (readY > 16) { readY += 16; }
          readY &= 0xF8;
          for (y = 0; y < 8; y++) {
            intensity = buf[buf_idx++];
            for (x = 0; x < 8; x++) {
              if ((intensity >> x) & 0x01)
                setGridLed(readX + x, readY + y, 15);
              else
                setGridLed(readX + x, readY + y, 0);
            }
          }
          break;

        case 0x15:            // /prefix/led/row x y d
          readX = buf[buf_idx++];
          while (readX > 16) { readX += 16; }
          readX &= 0xF8;
          readY     = buf[buf_idx++];
          intensity = buf[buf_idx++];
          for (x = 0; x < 8; x++) {
            if ((intensity >> x) & 0x01)
              setGridLed(readX + x, readY, 15);
            else
              setGridLed(readX + x, readY, 0);
          }
          break;

        case 0x16:            // /prefix/led/col x y d
          readX = buf[buf_idx++];
          readY = buf[buf_idx++];
          while (readY > 16) { readY += 16; }
          readY &= 0xF8;
          intensity = buf[buf_idx++];
          for (y = 0; y < 8; y++) {
            if ((intensity >> y) & 0x01)
              setGridLed(readX, readY + y, 15);
            else
              setGridLed(readX, readY + y, 0);
          }
          break;

        case 0x17:            // /prefix/led/intensity i
          intensity = buf[buf_idx++];
          if (intensity > 15) break;
          setAllLEDs(intensity);
          break;

        case 0x18:            // /prefix/led/level/set x y i
          readX     = buf[buf_idx++];
          readY     = buf[buf_idx++];
          intensity = buf[buf_idx++];
          setGridLed(readX, readY, intensity);
          break;

        case 0x19:            // /prefix/led/level/all s
          intensity = buf[buf_idx++];
          if (intensity > 15) break;
          setAllLEDs(intensity);
          break;

        case 0x1A:            // /prefix/led/level/map x y d[64]
          readX = buf[buf_idx++];
          while (readX > 16) { readX += 16; }
          readX &= 0xF8;
          readY = buf[buf_idx++];
          while (readY > 16) { readY += 16; }
          readY &= 0xF8;
          z = 0;
          for (y = 0; y < 8; y++) {
            for (x = 0; x < 8; x++) {
              if (z % 2 == 0) {
                intensity = buf[buf_idx++];
                if (((intensity >> 4) & 0x0F) > variMonoThresh)
                  setGridLed(readX + x, readY + y, (intensity >> 4) & 0x0F);
                else
                  setGridLed(readX + x, readY + y, 0);
              } else {
                if ((intensity & 0x0F) > variMonoThresh)
                  setGridLed(readX + x, readY + y, intensity & 0x0F);
                else
                  setGridLed(readX + x, readY + y, 0);
              }
              z++;
            }
          }
          break;

        case 0x1B:            // /prefix/led/level/row x y d[8]
          readX = buf[buf_idx++];
          while (readX > 16) { readX += 16; }
          readX &= 0xF8;
          readY = buf[buf_idx++];
          while (readY > 16) { readY += 16; }
          readY &= 0xF8;
          for (x = 0; x < 8; x++) {
            if (x % 2 == 0) {
              intensity = buf[buf_idx++];
              if ((intensity >> 4 & 0x0F) > variMonoThresh)
                setGridLed(readX + x, readY, (intensity >> 4) & 0x0F);
              else
                setGridLed(readX + x, readY, 0);
            } else {
              if ((intensity & 0x0F) > variMonoThresh)
                setGridLed(readX + x, readY, intensity & 0x0F);
              else
                setGridLed(readX + x, readY, 0);
            }
          }
          break;

        case 0x1C:            // /prefix/led/level/col x y d[8]
          readX = buf[buf_idx++];
          while (readX > 16) { readX += 16; }
          readX &= 0xF8;
          readY = buf[buf_idx++];
          while (readY > 16) { readY += 16; }
          readY &= 0xF8;
          for (y = 0; y < 8; y++) {
            if (y % 2 == 0) {
              intensity = buf[buf_idx++];
              if ((intensity >> 4 & 0x0F) > variMonoThresh)
                setGridLed(readX, readY + y, (intensity >> 4) & 0x0F);
              else
                setGridLed(readX, readY + y, 0);
            } else {
              if ((intensity & 0x0F) > variMonoThresh)
                setGridLed(readX, readY + y, intensity & 0x0F);
              else
                setGridLed(readX, readY + y, 0);
            }
          }
          break;

    // 0x20 and 0x21 are key inputs (grid)

        case 0x20:            // key-grid / key up
            gridKeyX = buf[buf_idx++];
            gridKeyY = buf[buf_idx++];
            addGridEvent(gridKeyX, gridKeyY, 0);
            break;

        case 0x21:            // key-grid / key down
            gridKeyX = buf[buf_idx++];
            gridKeyY = buf[buf_idx++];
            addGridEvent(gridKeyX, gridKeyY, 1);
            break;

        // 0x5x are encoder
        case 0x50:            // enc / delta
            index = buf[buf_idx++];
            delta = (int8_t)buf[buf_idx++];
            addArcEvent(index, delta);
            break;

        case 0x51:            // enc / key up
            n = buf[buf_idx++];
            break;

        case 0x52:            // enc / key down
            n = buf[buf_idx++];
            break;

        case 0x80:            // tilt active
            buf_idx += 9;
            break;

        case 0x81:            // tilt data
            buf_idx += 8;
            break;

        // 0x90 variable 64 LED ring
        case 0x90:            // ring / set n x a
          readN = buf[buf_idx++];
          readX = buf[buf_idx++];
          readA = buf[buf_idx++];
          setArcLed(readN, readX, readA);
          break;

        case 0x91:            // ring / all n a
          readN = buf[buf_idx++];
          readA = buf[buf_idx++];
          for (int q = 0; q < 64; q++)
            setArcLed(readN, q, readA);
          break;

        case 0x92:            // ring / map n d[32]
          readN = buf[buf_idx++];
          for (y = 0; y < 64; y++) {
            if (y % 2 == 0) {
              intensity = buf[buf_idx++];
              if ((intensity >> 4 & 0x0F) > 0)
                setArcLed(readN, y, (intensity >> 4 & 0x0F));
              else
                setArcLed(readN, y, 0);
            } else {
              if ((intensity & 0x0F) > 0)
                setArcLed(readN, y, (intensity & 0x0F));
              else
                setArcLed(readN, y, 0);
            }
          }
          break;

        case 0x93:            // ring / range n x1 x2 a
          readN = buf[buf_idx++];
          readX = buf[buf_idx++];  // x1
          readY = buf[buf_idx++];  // x2
          readA = buf[buf_idx++];
          //memset(led_array[readN],0,sizeof(led_array[readN]));
      
          if (readX < readY){
            for (y = readX; y <= readY; y++) {
              setArcLed(readN, y, readA);
            }
          }else{
            // wrapping
            for (y = readX; y < 64; y++) {
              setArcLed(readN, y, readA);
            }
            for (x = 0; x <= readY; x++) {
              setArcLed(readN, x, readA);
            }
          }
          //note:   set range x1-x2 (inclusive) to a. wrapping supported, ie. set range 60,4 would set values 60,61,62,63,0,1,2,3,4. 
          // always positive direction sweep. ie. 4,10 = 4,5,6,7,8,9,10 whereas 10,4 = 10,11,12,13...63,0,1,2,3,4 
         break;

        default: 
          break;
    }
}

void MonomeEventQueue::addGridEvent(uint8_t x, uint8_t y, uint8_t pressed) {
    if (gridEventCount >= MAXEVENTCOUNT) return;
    uint8_t ind = (gridFirstEvent + gridEventCount) % MAXEVENTCOUNT;
    gridEvents[ind].x = x;
    gridEvents[ind].y = y;
    gridEvents[ind].pressed = pressed;
    gridEventCount++;
}

bool MonomeEventQueue::gridEventAvailable() {
    return gridEventCount > 0;
}

MonomeGridEvent MonomeEventQueue::readGridEvent() {
    if (gridEventCount == 0) return emptyGridEvent;
    gridEventCount--;
    uint8_t index = gridFirstEvent;
    gridFirstEvent = (gridFirstEvent + 1) % MAXEVENTCOUNT;
    return gridEvents[index];
}

void MonomeEventQueue::addArcEvent(uint8_t index, int8_t delta) {
    if (arcEventCount >= MAXEVENTCOUNT) return;
    uint8_t ind = (arcFirstEvent + arcEventCount) % MAXEVENTCOUNT;
    arcEvents[ind].index = index;
    arcEvents[ind].delta = delta;
    arcEventCount++;
}

bool MonomeEventQueue::arcEventAvailable() {
    return arcEventCount > 0;
}

MonomeArcEvent MonomeEventQueue::readArcEvent() {
    if (arcEventCount == 0) return emptyArcEvent;
    arcEventCount--;
    uint8_t index = arcFirstEvent;
    arcFirstEvent = (arcFirstEvent + 1) % MAXEVENTCOUNT;
    return arcEvents[index];
}

void MonomeEventQueue::sendArcDelta(uint8_t index, int8_t delta) {
    /*
    Serial.print("Encoder:");
    Serial.print(index);
    Serial.print(" ");
    Serial.print(delta);
    Serial.println(" ");
    */
    tud_cdc_write_char((uint8_t)0x50);
    tud_cdc_write_char((uint8_t)index);
    tud_cdc_write_char((int8_t)delta);
    tud_cdc_n_write_flush(0);
 
    /*
    byte buf[3];
    buf[0] = 0x50;
    buf[1] = index;
    buf[2] = delta;
    tud_cdc_write_char(buf, sizeof(buf));
    */
}

void MonomeEventQueue::sendArcKey(uint8_t index, uint8_t pressed) {
    /*
    Serial.print("key:");
    Serial.print(index);
    Serial.print(" ");
    Serial.println(pressed);
    */
    uint8_t buf[2];
    if (pressed == 1){
      buf[0] = 0x52;
    }else{
      buf[0] = 0x51;
    }
    buf[1] = index;
    tud_cdc_write_char(buf[0]);
    tud_cdc_write_char(buf[1]);
    tud_cdc_n_write_flush(0);
}

void MonomeEventQueue::sendGridKey(uint8_t x, uint8_t y, uint8_t pressed) {    
    uint8_t buf[2];
    if (pressed == 1){
      buf[0] = 0x21;
    }else{
      buf[0] = 0x20;
    }
    tud_cdc_write_char((uint8_t)buf[0]);
    tud_cdc_write_char((uint8_t)x);
    tud_cdc_write_char((uint8_t)y);
    tud_cdc_n_write_flush(0);
}

void MonomeEventQueue::sendTiltEvent(uint8_t n,int8_t xh,int8_t xl,int8_t yh,int8_t yl,int8_t zh,int8_t zl)
{    
    tud_cdc_write_char((uint8_t)0x81);
    tud_cdc_write_char((uint8_t)n);
    tud_cdc_write_char((int8_t)xh);
    tud_cdc_write_char((int8_t)xl);
    tud_cdc_write_char((int8_t)yh);
    tud_cdc_write_char((int8_t)yl);
    tud_cdc_write_char((int8_t)zh);
    tud_cdc_write_char((int8_t)zl);
    tud_cdc_n_write_flush(0);
 }
