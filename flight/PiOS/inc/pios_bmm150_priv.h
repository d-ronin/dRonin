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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#define BMM150_REG_MAG_HALL_RESISTANCE_MSB 0x49
#define BMM150_REG_MAG_INT_STATUS 0x4A
#define BMM150_REG_MAG_POWER_CONTROL 0x4B
#define BMM150_VAL_MAG_POWER_CONTROL_SOFTRESET 0x82
/* According to datasheet, full POR behavior attained by poweroff and
 * going back to poweron */
#define BMM150_VAL_MAG_POWER_CONTROL_POWERON 0x01
#define BMM150_VAL_MAG_POWER_CONTROL_POWEROFF 0x00
#define BMM150_REG_MAG_OPERATION_MODE 0x4C
#define BMM150_REG_MAG_INTERRUPT_SETTINGS 0x4D
#define BMM150_REG_MAG_INTERRUPT_SETTINGS_AXES_ENABLE_BITS 0x4E
#define BMM150_REG_MAG_LOWTHRESH_INTERRUPT_SETTING 0x4F
#define BMM150_REG_MAG_HIGHTHRESH_INTERRUPT_SETTING 0x50
#define BMM150_REG_MAG_X_Y_AXIS_REP 0x51
#define BMM150_REG_MAG_Z_AXIS_REP 0x52

#define BMM050_INIT_VALUE (0)
#define BMM050_OVERFLOW_OUTPUT_FLOAT	0.0f
#define BMM050_FLIP_OVERFLOW_ADCVAL		-4096
#define BMM050_HALL_OVERFLOW_ADCVAL		-16384

#endif /* PIOS_BMM150_PRIV_H */

/** 
  * @}
  * @}
  */

