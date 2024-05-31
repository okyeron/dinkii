# dink-ii 

dink-ii is an RP2040 based microcontroller designed for plug-n-play use with STEMMA-QT compatible i2c devices.

IMPORTANT - by default dink-ii sends 5v over the STEMMA-QT connector. If you need 3.3v for your device, see the jumper section below.

dink-ii uses a standard STEMMA-QT (JST-SH) connector for i2c.

## Programming notes

i2c is set to the `i2c1` bus, so use `Wire1` in Arduino. i2c Pins are `SDA = 2` and `SCL = 3`

in Arduino you would configure this in `setup()` with
```
	Wire1.setSDA(I2C_SDA);
	Wire1.setSCL(I2C_SCL);
```


## 5v jumper

The STEMMA-QT connector on dink-ii defaults to using 5v. If you need  3.3v instead for some other project, there is a 3-way jumper on the bottom of the board. For 3.3v, cut the trace between the center pad and the 5v pad, and then re-solder a bridge from the center pad to the 3.3v pad.