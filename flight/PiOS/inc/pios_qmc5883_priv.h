/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_QMC5883 QMC5883 Functions
 * @brief Deals with the hardware interface to the magnetometers
 * @{
 *
 * @file       pios_qmc5883.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      QMC5883 functions header.
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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */

#ifndef PIOS_QMC5883_H
#define PIOS_QMC5883_H

#include <stdint.h>
#include <stdbool.h>

/* QMC5883 Addresses */
#define PIOS_QMC5883_I2C_ADDR             0x0D
#define PIOS_QMC5883_I2C_READ_ADDR        0x1B
#define PIOS_QMC5883_I2C_WRITE_ADDR       0x1A

#define PIOS_QMC5883_DATAOUT_XLSB_REG     0x00
#define PIOS_QMC5883_DATAOUT_XMSB_REG     0x01
#define PIOS_QMC5883_DATAOUT_YLSB_REG     0x02
#define PIOS_QMC5883_DATAOUT_YMSB_REG     0x03
#define PIOS_QMC5883_DATAOUT_ZLSB_REG     0x04
#define PIOS_QMC5883_DATAOUT_ZMSB_REG     0x05
#define PIOS_QMC5883_DATAOUT_STATUS_REG   0x06
#define PIOS_QMC5883_DATAOUT_TLSB_REG     0x07
#define PIOS_QMC5883_DATAOUT_TMSB_REG     0x08
#define PIOS_QMC5883_CONTROL_REG_1        0x09
#define PIOS_QMC5883_CONTROL_REG_2        0x0A
#define PIOS_QMC5883_SET_RESET_PERIOD_REG 0x0B
#define PIOS_QMC5883_ID_REG               0x0D

#define PIOS_QMC5883_DRDY                 0x01
#define PIOS_QMC5883_OVL                  0x02
#define PIOS_QMC5883_DOR                  0x04

#define PIOS_QMC5883_MODE_STANDBY         0x00
#define PIOS_QMC5883_MODE_CONTINUOUS      0x01

#define PIOS_QMC5883_ODR_10HZ             0b00 << 2
#define PIOS_QMC5883_ODR_50HZ             0b01 << 2
#define PIOS_QMC5883_ODR_100HZ            0b10 << 2
#define PIOS_QMC5883_ODR_200HZ            0b11 << 2

#define PIOS_QMC5883_RNG_2G               0b00 << 4
#define PIOS_QMC5883_RNG_8G               0b01 << 4

#define PIOS_QMC5883_OSR_512              0b00 << 6
#define PIOS_QMC5883_OSR_256              0b01 << 6
#define PIOS_QMC5883_OSR_128              0b10 << 6
#define PIOS_QMC5883_OSR_64               0b11 << 6

#define PIOS_QMC5883_INT_ENB              0x01
#define PIOS_QMC5883_ROL_PNT              0x40
#define PIOS_QMC5883_SOFT_RESET           0x80

#define PIOS_QMC5883_SET_RESET_PERIOD     0x01

#define PIOS_QMC5883_ID                   0xFF

enum pios_qmc5883_orientation {
	// clockwise rotation from board forward while looking at top side
	// 0 degree is chip mark on upper left corner
	PIOS_QMC5883_TOP_0DEG,
	PIOS_QMC5883_TOP_90DEG,
	PIOS_QMC5883_TOP_180DEG,
	PIOS_QMC5883_TOP_270DEG,
	// clockwise rotation from board forward while looking at bottom side
	// 0 degree is chip mark on upper left corner
	PIOS_QMC5883_BOTTOM_0DEG,
	PIOS_QMC5883_BOTTOM_90DEG,
	PIOS_QMC5883_BOTTOM_180DEG,
	PIOS_QMC5883_BOTTOM_270DEG
};

struct pios_qmc5883_cfg {
	const struct pios_exti_cfg * exti_cfg; /* Pointer to the EXTI configuration */
	uint8_t ODR;                           /* Output Data Rate */
	uint8_t Rng;		                   /* Rng Configuration */
	uint8_t Mode;
	enum pios_qmc5883_orientation Default_Orientation;
};

struct pios_qmc5883_data {
	int16_t mag_x;
	int16_t mag_y;
	int16_t mag_z;
};

/* Public Functions */
extern int32_t PIOS_QMC5883_Init(pios_i2c_t i2c_id,	const struct pios_qmc5883_cfg * cfg);
extern int32_t PIOS_QMC5883_Test(void);
extern int32_t PIOS_QMC5883_SetOrientation(enum pios_qmc5883_orientation orientation);
extern bool PIOS_QMC5883_IRQHandler();
#endif /* PIOS_QMC5883_H */

/** 
  * @}
  * @}
  */
