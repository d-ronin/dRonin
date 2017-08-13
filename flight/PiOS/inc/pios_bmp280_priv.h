/**
 ******************************************************************************
 * @file       pios_bmp280.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMP280 BMP280 Functions
 * @{
 * @brief Driver for BMP280
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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef PIOS_BMP280_PRIV_H
#define PIOS_BMP280_PRIV_H

#define BMP280_STANDARD_RESOLUTION		0x2D
#define BMP280_HIGH_RESOLUTION			0x31
#define BMP280_ULTRA_HIGH_RESOLUTION	0x55

//! Configuration structure for the BMP280 driver
struct pios_bmp280_cfg {

	//! The oversampling setting for the baro, higher produces
	//! less frequent cleaner data.  This data byte is used
	//! to trigger the conversion.
	uint8_t oversampling;
};

int32_t PIOS_BMP280_Init(const struct pios_bmp280_cfg *cfg, pios_i2c_t i2c_device);
int32_t PIOS_BMP280_SPI_Init(const struct pios_bmp280_cfg *cfg, pios_spi_t spi_device,
	uint32_t spi_slave);

#endif /* PIOS_BMP280_PRIV_H */

/**
  * @}
  * @}
  */
