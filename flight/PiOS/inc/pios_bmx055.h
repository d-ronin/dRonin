/**
 ******************************************************************************
 * @file       pios_bmx055.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMX055
 * @{
 * @brief Driver for Bosch BMX055 IMU Sensor
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
 */

#ifndef PIOS_BMX055_H
#define PIOS_BMX055_H

#include "pios.h"

enum pios_bmx055_orientation { // clockwise rotation from board forward
	PIOS_BMX_TOP_0DEG    = 0x00,
	PIOS_BMX_TOP_90DEG   = 0x01,
	PIOS_BMX_TOP_180DEG  = 0x02,
	PIOS_BMX_TOP_270DEG  = 0x03,
	PIOS_BMX_BOTTOM_0DEG  = 0x04,
	PIOS_BMX_BOTTOM_90DEG  = 0x05,
	PIOS_BMX_BOTTOM_180DEG  = 0x06,
	PIOS_BMX_BOTTOM_270DEG  = 0x07,
};

struct pios_bmx055_cfg {
//	const struct pios_exti_cfg *exti_cfg; /* Pointer to the EXTI configuration */

	enum pios_bmx055_orientation orientation;
	bool skip_startup_irq_check;
};

typedef struct pios_bmx055_dev * pios_bmx055_dev_t;

/**
 * @brief Initialize the BMX-xxxx 6/9-axis sensor on SPI
 * @return 0 for success, -1 for failure to allocate, -10 for failure to get irq
 */
int32_t PIOS_BMX055_SPI_Init(pios_bmx055_dev_t *dev, pios_spi_t spi_id,
		uint32_t slave_gyro, uint32_t slave_accel,
		const struct pios_bmx055_cfg *cfg);

#if 0
/**
 * @brief The IMU interrupt handler.
 * Fetches new data from the IMU.
 */
bool PIOS_BMX055_IRQHandler(void);
#endif

#endif /* PIOS_BMX055_H */

/** 
  * @}
  * @}
  */
