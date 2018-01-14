# hc-laser-beam-timer

Firmware for an Arduino-based laser break beam timer.  I use it for timing line following robots, allowing for analysis of adjustments to line following algorithms and hardware configurations.

## Production

The produciton code is in the Trinket folder.  The production model runs on Adafruit Trinket, requiring some changes to reduce code and memory size.  Connection to LCD module over I2C using TinyWireM library.

Some changes were required to LiquidCrystal_I2C to make it use TinyWireM.  The main line of LiquidCrystal_I2C has moved on quite a bit since I worked on this code, so the edited copy in use for Laser Beam Timer is in the libs directory.

The general idea is to wrap all the references to Wire.h or Wire objects in preprocessor conditionals taking advantage of the ARDUINO_AVR_TRINKET3 variable that is set when you select the Trinket board in the Arduino IDE.

For example, if the original LiquidCrystal_I2C had...

```c++
#include <Wire.h>
```
...changes to...
```c++
#ifdef ARDUINO_AVR_TRINKET3
#include <TinyWireM.h>
#else
#include <Wire.h>
#endif
```

Or another example...
```c++
Wire.begin();
```
...changes to...
```c++
#ifdef ARDUINO_AVR_TRINKET3
	TinyWireM.begin();
#else
	Wire.begin();
#endif
```
The .rrb files are PCB layout files from the no-longer-in-existance Copper Connection software.

## Development Prototype

Uno folder is deprecated. The prototype model ran on Arduino Uno, which allows larger code size.  Also connected to LCD via Hitachi interface because Uno has pins to spare.
