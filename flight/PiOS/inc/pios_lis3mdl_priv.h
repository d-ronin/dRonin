/**
 ******************************************************************************
 * @file       pios_bmm150_priv.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_LIS3MDL
 * @{
 * @brief  Driver for LIS3MDL magnetometer
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

#ifndef PIOS_LIS3MDL_PRIV_H
#define PIOS_LIS3MDL_PRIV_H

#include "pios_lis3mdl.h"

enum pios_lis3_regs {
	LIS_REG_MAG_WHO_AM_I = 0x0f,
	LIS_REG_MAG_CTRL1 = 0x20,
	LIS_REG_MAG_CTRL2,
	LIS_REG_MAG_CTRL3,
	LIS_REG_MAG_CTRL4,
	LIS_REG_MAG_CTRL5,
	LIS_REG_MAG_STATUS = 0x27,
	LIS_REG_MAG_OUTX_L,
	LIS_REG_MAG_OUTX_H,
	LIS_REG_MAG_OUTY_L,
	LIS_REG_MAG_OUTY_H,
	LIS_REG_MAG_OUTZ_L,
	LIS_REG_MAG_OUTZ_H,
	LIS_REG_MAG_TEMPOUT_L,
	LIS_REG_MAG_TEMPOUT_H,
	LIS_REG_MAG_INT_CFG,
	LIS_REG_MAG_INT_SRC,
	LIS_REG_MAG_INT_THS_L,
	LIS_REG_MAG_INT_THS_H
};

#define LIS_ADDRESS_READ 0x80
#define LIS_ADDRESS_AUTOINCREMENT 0x40

#define LIS_WHO_AM_I_VAL 0x3d

#define LIS_CTRL1_TEMPEN 0x80
#define LIS_CTRL1_OPMODE_MASK 0x60

#define LIS_CTRL1_OPMODE_UHP 0x60		/* Ultra high perofrmance */
#define LIS_CTRL1_ODR_MASK 0x1c
#define LIS_CTRL1_ODR_80 0x1c		/* Mfr recommended */
#define LIS_CTRL1_ODR_40 0x18
#define LIS_CTRL1_ODR_20 0x14

#define LIS_CTRL1_FASTODR 0x02		/* generates 155Hz with UHP */
#define LIS_CTRL1_SELFTEST 0x01

#define LIS_CTRL2_FS_MASK 0x60
#define LIS_CTRL2_FS_16GAU 0x60
#define LIS_CTRL2_FS_12GAU 0x40		/* Mfr recommended; 2281
						 * counts/gauss.  typ RMS
						 * noise seems to be about
						 * 3 milligauss or 7 counts
						 */
#define LIS_RANGE_12GAU_COUNTS_PER_MGAU 2.281f

#define LIS_CTRL2_FS_8GAU 0x20
#define LIS_CTRL2_FS_4GAU 0x00

#define LIS_CTRL2_REBOOT 0x04
#define LIS_CTRL2_SOFTRST 0x02

#define LIS_CTRL3_LP 0x20
#define LIS_CTRL3_SPITHREEWIRE 0x04
#define LIS_CTRL3_MODE_MASK 0x03

#define LIS_CTRL3_MODE_CONTINUOUS 0x00
#define LIS_CTRL3_MODE_SINGLE 0x01
#define LIS_CTRL3_MODE_POWERDOWN 0x03

#define LIS_CTRL4_OPMODEZ_MASK 0x0c
#define LIS_CTRL4_OPMODEZ_UHP 0x0c
/* Endianness is also here, but default value here corresponds to register
 * map and microarchitecture endianness (x86=LE, ARM as we use=LE, etc).
 */

#define LIS_CTRL5_FASTREAD 0x80
#define LIS_CTRL5_BDU 0x40

#define LIS_STATUS_ZYXOR 0x80
#define LIS_STATUS_ZOR 0x40
#define LIS_STATUS_YOR 0x20
#define LIS_STATUS_XOR 0x10
#define LIS_STATUS_ZYXDA 0x08
#define LIS_STATUS_ZDA 0x04
#define LIS_STATUS_YDA 0x02
#define LIS_STATUS_XDA 0x01

#endif /* PIOS_LIS3MDL_PRIV_H */

/** 
  * @}
  * @}
  */
