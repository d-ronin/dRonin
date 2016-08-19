/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMP280 BMP085 Functions
 * @brief Hardware functions to deal with the altitude pressure sensor
 * @{
 *
 * @file       pios_bmp280_priv.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @brief      BMP280 functions header.
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef PIOS_BMP280_PRIV_H
#define PIOS_BMP280_PRIV_H

#define BMP280_STANDARD_RESOLUTION		0x2D
#define BMP280_HIGH_RESOLUTION			0x31
#define BMP280_ULTRA_HIGH_RESOLUTION	0x55

//! Configuration structure for the BMP085 driver
struct pios_bmp280_cfg {

	//! The oversampling setting for the baro, higher produces
	//! less frequent cleaner data.  This data byte is used
	//! to trigger the conversion.
	uint8_t oversampling;

	//! How many samples of pressure for each temperature measurement
    uint32_t temperature_interleaving;
};

int32_t PIOS_BMP280_Init(const struct pios_bmp280_cfg *cfg, int32_t i2c_device);


#endif /* PIOS_BMP280_PRIV_H */

/**
  * @}
  * @}
  */