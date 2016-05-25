#include "Arduino.h"
#include <I2C.h> // https://github.com/rambo/I2C
#include <SimpleBMP180I2C.h>

/*
 * This is an example for the BMP180 barometric pressure sensor
 * The BMP180 can be set to 4 hardware sampling modes:
 *
 * #	Mode				samples		conversion	RMS noise
 * 										time [ms]	[hPa]
 * 0	ultra low power			1		 4.5		0.06
 * 1	standard				2		 7.5		0.05
 * 2	high resolution			4		13.5		0.04
 * 3	ultra high resolution	8		25.5		0.03
 *
 * Example: baro.getPressure(3) gets the pressure in ultra high
 * resolution. The default mode is 1 (standard)
 *
 * Example is for the GY-68. The GY-68 got a 3.3V regulator and the
 * pullup resistors are attached to the 3.3V supply.
 * This means that the internal pullups from the Arduino must be
 * disabled to prevent overloading the BMP180 sensor.
 * this doesn't work for the Mega2560 as these got pullup resistors
 * on the board. In that case a level shifter is required.
 * This example compiles to about 400 bytes less code with respect
 * to the Wire library and won't freeze when the sensor is disconnected
 */

SimpleBMP180I2C baro;

void setup() {
  Serial.begin(9600);
  I2c.begin();
  I2c.pullup(0);
  I2c.setSpeed(1);
  I2c.timeOut(10);

  if (!baro.begin()) {
	Serial.println(F("No BMP180 sensor found!"));
	while (1) {}
  }
}

void loop() {
	Serial.print(F("Pressure = "));
	Serial.print(baro.getPressure(), DEC);
	Serial.println(F(" Pa"));

	Serial.print(F("Temperature = "));
	Serial.print(baro.getLastTemperature(), DEC);
	Serial.println(F(" dC"));

	Serial.println();
	delay(1000);
}
