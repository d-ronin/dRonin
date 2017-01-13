/**
 ******************************************************************************
 * @file       pios_bmm150.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMX055
 * @{
 * @brief Driver for Bosch BMM150 IMU Sensor
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

#ifndef PIOS_BMM150_H
#define PIOS_BMM150_H

#include "pios.h"

enum pios_bmm150_orientation { // clockwise rotation from board forward
	PIOS_BMM_TOP_0DEG    = 0x00,
	PIOS_BMM_TOP_90DEG   = 0x01,
	PIOS_BMM_TOP_180DEG  = 0x02,
	PIOS_BMM_TOP_270DEG  = 0x03,
	PIOS_BMM_BOTTOM_0DEG  = 0x04,
	PIOS_BMM_BOTTOM_90DEG  = 0x05,
	PIOS_BMM_BOTTOM_180DEG  = 0x06,
	PIOS_BMM_BOTTOM_270DEG  = 0x07,
};

struct pios_bmm150_cfg {
	enum pios_bmm150_orientation orientation;
};

typedef struct pios_bmm150_dev * pios_bmm150_dev_t;

/**
 * @brief Initialize the BMM-xxxx 6/9-axis sensor on SPI
 * @return 0 for success, -1 for failure to allocate, -10 for failure to get irq
 */
int32_t PIOS_BMM150_SPI_Init(pios_bmm150_dev_t *dev, uint32_t spi_id, uint32_t slave_mag, const struct pios_bmm150_cfg *cfg);

#endif /* PIOS_BMM150_H */

/** 
  * @}
  * @}
  */
