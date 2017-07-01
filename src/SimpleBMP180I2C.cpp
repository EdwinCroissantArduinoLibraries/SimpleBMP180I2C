/*
 * BMP180 barometric pressure sensor library for the Arduino
 * microcontroller.
 *
 * uses the I2C library from Wayne Truchsess
 *
 * Copyright (C) 2016 Edwin Croissant
 *
 *  This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See the README.md file for additional information.
 */

#include "SimpleBMP180I2C.h"

SimpleBMP180I2C::SimpleBMP180I2C() {
	_lastTemp = -2732;
}

/*
 * Check the device and retrieves the calibration data
 * Returns false if the operation fails.
 */
boolean SimpleBMP180I2C::begin() {
	/* check device */
	if (I2c.read(I2CAddr, id, 1) != 0)
		return false;
	if (I2c.receive() != deviceId)
		return false;

	/* read calibration data */
	if (I2c.read(I2CAddr, calData, 22) != 0)
		return false;
	// need to swap the MSB and LSB
	for (int i = 0; i < 22; ++i) {
		if (i % 2)
			_calData.calArray[i - 1] = I2c.receive();
		else
			_calData.calArray[i + 1] = I2c.receive();
	}
	return true;
}

/*
 * Initiate a temperature and pressure reading and returns
 * the pressure in Pascal. If there is a I2C error returns 0.
 * The BMP180 can be set to 4 hardware sampling modes:
 * #	Mode				samples		conversion	RMS noise
 * 										time [ms]	[hPa]
 * 0	ultra low power			1		 4.5		0.06
 * 1	standard				2		 7.5		0.05
 * 2	high resolution			4		13.5		0.04
 * 3	ultra high resolution	8		25.5		0.03
 *
 * Default mode is 1
 */
int32_t SimpleBMP180I2C::getPressure(uint8_t mode) {
	int32_t B3, B5, B6, X1, X2, X3, p;
	uint32_t B4, B7;
	union {
		int32_t int32;
		struct {
			uint8_t LLSB, LSB, MSB, MMSB;
		};
	} UT, UP; //uncompensated temperature and pressure

	// clip the mode
	if (mode > 3)
		mode = 3;

	/* Get the temperature first */
	if (I2c.write(I2CAddr, control, readTemp) != 0)
		goto onError;

	delay(5);

	if (I2c.read(I2CAddr, data, 2) != 0)
		goto onError;
	UT.MMSB = 0;
	UT.MSB = 0;
	UT.LSB = I2c.receive();
	UT.LLSB = I2c.receive();

	// Calculate the temperature

	X1 = (UT.int32 - (int32_t) _calData.ac6) * ((int32_t) _calData.ac5) >> 15;
	X2 = ((int32_t) _calData.mc << 11) / (X1 + (int32_t) _calData.md);
	B5 = X1 + X2;
	_lastTemp = (B5 + 8) >> 4; // in milliCelsius

	/* Now get the pressure */
	if (I2c.write(I2CAddr, control, readPress + (mode << 6)) != 0)
		goto onError;

	switch (mode) {
	case 0:
		delay(5);
		break;
	case 1:
		delay(8);
		break;
	case 2:
		delay(14);
		break;
	case 3:
		delay(26);
	}
	if (I2c.read(I2CAddr, data, 3) != 0)
		goto onError;
	UP.MMSB = 0;
	UP.MSB = I2c.receive();
	UP.LSB = I2c.receive();
	UP.LLSB = I2c.receive();
	UP.int32 >>= (8 - mode);

	// and calculate the pressure
	B6 = B5 - 4000;
	X1 = ((int32_t) _calData.b2 * ((B6 * B6) >> 12)) >> 11;
	X2 = ((int32_t) _calData.ac2 * B6) >> 11;
	X3 = X1 + X2;
	B3 = ((((int32_t) _calData.ac1 * 4 + X3) << mode) + 2) / 4;

	X1 = ((int32_t) _calData.ac3 * B6) >> 13;
	X2 = ((int32_t) _calData.b1 * ((B6 * B6) >> 12)) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	B4 = ((uint32_t) _calData.ac4 * (uint32_t) (X3 + 32768)) >> 15;
	B7 = ((uint32_t) UP.int32 - B3) * (uint32_t) (50000UL >> mode);

	if (B7 < 0x80000000) {
		p = (B7 * 2) / B4;
	} else {
		p = (B7 / B4) * 2;
	}
	X1 = (p >> 8) * (p >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * p) >> 16;

	p = p + ((X1 + X2 + (int32_t) 3791) >> 4);

	return p; // in Pascal

	onError: _lastTemp = -2732;
	return 0;
}

/*
 * Retrieves the sensor temperature in deciCelsius as
 * measured during the pressure reading.  If there was
 * a I2C error returns -2732.
 */
int16_t SimpleBMP180I2C::getLastTemperature(void) {
	return _lastTemp;
}
