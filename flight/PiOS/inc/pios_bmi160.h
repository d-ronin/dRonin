/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMI160 BMI160 Functions
 * @brief Hardware functions to deal with the 6DOF gyro / accel sensor
 * @{
 *
 * @file       pios_bmi160.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @brief      BMI160 Gyro / Accel Sensor Routines
 * @see        The GNU Public License (GPL) Version 3
 ******************************************************************************/

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


#ifndef PIOS_BMI160_H
#define PIOS_BMI160_H

enum pios_bmi160_orientation { // clockwise rotation from board forward
	PIOS_BMI160_TOP_0DEG,
	PIOS_BMI160_TOP_90DEG,
	PIOS_BMI160_TOP_180DEG,
	PIOS_BMI160_TOP_270DEG,
	PIOS_BMI160_BOTTOM_0DEG,
	PIOS_BMI160_BOTTOM_90DEG,
	PIOS_BMI160_BOTTOM_180DEG,
	PIOS_BMI160_BOTTOM_270DEG,
};

enum pios_bmi160_odr {
	PIOS_BMI160_ODR_800_Hz = 0x0B,
	PIOS_BMI160_ODR_1600_Hz = 0x0C,
};

enum pios_bmi160_acc_range {
	PIOS_BMI160_RANGE_2G = 0x03,
	PIOS_BMI160_RANGE_4G = 0x05,
	PIOS_BMI160_RANGE_8G = 0x08,
	PIOS_BMI160_RANGE_16G = 0x0C,
};

enum pios_bmi160_gyro_range {
	PIOS_BMI160_RANGE_125DPS = 0x04,
	PIOS_BMI160_RANGE_250DPS = 0x03,
	PIOS_BMI160_RANGE_500DPS = 0x02,
	PIOS_BMI160_RANGE_1000DPS = 0x01,
	PIOS_BMI160_RANGE_2000DPS = 0x00,
};

struct pios_bmi160_cfg {
	const struct pios_exti_cfg *exti_cfg; /* Pointer to the EXTI configuration */
	enum pios_bmi160_orientation orientation;
	enum pios_bmi160_odr odr;
	enum pios_bmi160_acc_range acc_range;
	enum pios_bmi160_gyro_range gyro_range;
	uint8_t temperature_interleaving;
};

/* Public Functions */
extern int32_t PIOS_BMI160_Init(uint32_t spi_id, uint32_t slave_num, const struct pios_bmi160_cfg *cfg, bool do_foc);
extern bool PIOS_BMI160_IRQHandler(void);

#endif /* PIOS_BMI160_H */
