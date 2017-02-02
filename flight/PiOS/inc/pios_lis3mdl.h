/**
 ******************************************************************************
 * @file       pios_lis3mdl.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_LIS3MDL
 * @{
 * @brief Driver for ST LIS3MDL Magnetometer
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

#ifndef PIOS_LIS3MDL_H
#define PIOS_LIS3MDL_H

#include "pios.h"

enum pios_lis3mdl_orientation { // clockwise rotation from board forward
	PIOS_LIS_TOP_0DEG    = 0x00,
	PIOS_LIS_TOP_90DEG   = 0x01,
	PIOS_LIS_TOP_180DEG  = 0x02,
	PIOS_LIS_TOP_270DEG  = 0x03,
	PIOS_LIS_BOTTOM_0DEG  = 0x04,
	PIOS_LIS_BOTTOM_90DEG  = 0x05,
	PIOS_LIS_BOTTOM_180DEG  = 0x06,
	PIOS_LIS_BOTTOM_270DEG  = 0x07,
};

struct pios_lis3mdl_cfg {
	enum pios_lis3mdl_orientation orientation;
};

typedef struct lis3mdl_dev * lis3mdl_dev_t;

/**
 * @brief Initialize the LIS3MDL mag
 * @return 0 for success, -1 for failure to allocate, -10 for failure to get irq
 */
int32_t PIOS_LIS3MDL_SPI_Init(lis3mdl_dev_t *dev, uint32_t spi_id, uint32_t slave_mag, const struct pios_lis3mdl_cfg *cfg);

#endif /* PIOS_LIS3MDL_H */

/** 
  * @}
  * @}
  */
