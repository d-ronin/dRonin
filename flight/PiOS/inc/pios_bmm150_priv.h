/**
 ******************************************************************************
 * @file       pios_bmm150_priv.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMX055
 * @{
 * @brief  Driver for Bosch BMM150 IMU Sensor  (part of BMX055)
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

#ifndef PIOS_BMM150_PRIV_H
#define PIOS_BMM150_PRIV_H

#include "pios_bmm150.h"

#define BMM150_REG_MAG_CHIPID 0x40
#define BMM150_VAL_MAG_CHIPID 0x32
#define BMM150_REG_MAG_X_LSB 0x42
#define BMM150_REG_MAG_X_MSB 0x43
#define BMM150_REG_MAG_Y_LSB 0x44
#define BMM150_REG_MAG_Y_MSB 0x45
#define BMM150_REG_MAG_Z_LSB 0x46
#define BMM150_REG_MAG_Z_MSB 0x47
#define BMM150_REG_MAG_HALL_RESISTANCE_LSB 0x48
#define BMM150_VAL_MAG_HALL_RESISTANCE_LSB_DRDY 0x01
#define BMM150_REG_MAG_HALL_RESISTANCE_MSB 0x49
#define BMM150_REG_MAG_INT_STATUS 0x4A
#define BMM150_REG_MAG_POWER_CONTROL 0x4B
#
#define BMM150_VAL_MAG_POWER_CONTROL_SOFTRESET 0x82
/* According to datasheet, full POR behavior attained by poweroff and
 * going back to poweron */
#define BMM150_VAL_MAG_POWER_CONTROL_POWERON 0x01
#define BMM150_VAL_MAG_POWER_CONTROL_POWEROFF 0x00
#define BMM150_REG_MAG_OPERATION_MODE 0x4C
#define BMM150_VAL_MAG_OPERATION_MODE_ODR30 0x38
#define BMM150_VAL_MAG_OPERATION_MODE_ODR25 0x30
#define BMM150_VAL_MAG_OPERATION_MODE_ODR20 0x28
#define BMM150_VAL_MAG_OPERATION_MODE_ODR15 0x20
#define BMM150_VAL_MAG_OPERATION_MODE_ODR10 0x00
#define BMM150_VAL_MAG_OPERATION_MODE_ODR8 0x18
#define BMM150_VAL_MAG_OPERATION_MODE_ODR6 0x10
#define BMM150_VAL_MAG_OPERATION_MODE_ODR2 0x08

#define BMM150_VAL_MAG_OPERATION_MODE_OPMODE_NORMAL 0x00
#define BMM150_VAL_MAG_OPERATION_MODE_OPMODE_FORCED 0x02
#define BMM150_VAL_MAG_OPERATION_MODE_OPMODE_SLEEP 0x06
#define BMM150_REG_MAG_INTERRUPT_SETTINGS 0x4D
#define BMM150_REG_MAG_INTERRUPT_SETTINGS_AXES_ENABLE_BITS 0x4E
#define BMM150_REG_MAG_LOWTHRESH_INTERRUPT_SETTING 0x4F
#define BMM150_REG_MAG_HIGHTHRESH_INTERRUPT_SETTING 0x50
#define BMM150_REG_MAG_X_Y_AXIS_REP 0x51
#define BMM150_REG_MAG_Z_AXIS_REP 0x52

/* trim registers */
#define BMM150_DIG_X1 0x5D
#define BMM150_DIG_Y1 0x5E
#define BMM150_DIG_Z4_LSB 0x62
#define BMM150_DIG_Z4_MSB 0x63
#define BMM150_DIG_X2 0x64
#define BMM150_DIG_Y2 0x65
#define BMM150_DIG_Z2_LSB 0x68
#define BMM150_DIG_Z2_MSB 0x69
#define BMM150_DIG_Z1_LSB 0x6A
#define BMM150_DIG_Z1_MSB 0x6B
#define BMM150_DIG_XYZ1_LSB 0x6C
#define BMM150_DIG_XYZ1_MSB 0x6D
#define BMM150_DIG_Z3_LSB 0x6E
#define BMM150_DIG_Z3_MSB 0x6F
#define BMM150_DIG_XY2 0x70
#define BMM150_DIG_XY1 0x71

#define BMM050_INIT_VALUE (0)
#define BMM050_OVERFLOW_OUTPUT_FLOAT	0.0f
#define BMM050_FLIP_OVERFLOW_ADCVAL		-4096
#define BMM050_HALL_OVERFLOW_ADCVAL		-16384

#endif /* PIOS_BMM150_PRIV_H */

/** 
  * @}
  * @}
  */

