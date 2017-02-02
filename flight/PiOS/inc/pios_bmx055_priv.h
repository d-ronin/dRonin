/**
 ******************************************************************************
 * @file       pios_bmx055_priv.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMX055
 * @{
 * @brief  Driver for Bosch BMX055 IMU Sensor
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

#ifndef PIOS_BMX055_PRIV_H
#define PIOS_BMX055_PRIV_H

#include "pios_bmx055.h"

#define BMX055_REG_ACC_CHIPID 0x00
#define BMX055_VAL_ACC_CHIPID 0xfa
#define BMX055_REG_ACC_X_LSB 0x02
#define BMX055_REG_ACC_X_MSB 0x03
#define BMX055_REG_ACC_Y_LSB 0x04
#define BMX055_REG_ACC_Y_MSB 0x05
#define BMX055_REG_ACC_Z_LSB 0x06
#define BMX055_REG_ACC_Z_MSB 0x07
#define BMX055_REG_ACC_TEMP 0x08
#define BMX055_ACC_TEMP_OFFSET 23
#define BMX055_REG_ACC_INT_STATUS_0 0x09
#define BMX055_REG_ACC_INT_STATUS_1 0x0a
#define BMX055_REG_ACC_INT_STATUS_2 0x0b
#define BMX055_REG_ACC_INT_STATUS_3 0x0c
#define BMX055_REG_ACC_FIFO_STATUS 0x0e
#define BMX055_REG_ACC_PMU_RANGE 0x0f
#define BMX055_VAL_ACC_PMU_RANGE_2G 0x03
#define BMX055_VAL_ACC_PMU_RANGE_4G 0x05
#define BMX055_VAL_ACC_PMU_RANGE_8G 0x08
#define BMX055_VAL_ACC_PMU_RANGE_16G 0x0c
#define BMX055_REG_ACC_PMU_BW 0x10
#define BMX055_VAL_ACC_PMU_BW_7HZ81 0x08
#define BMX055_VAL_ACC_PMU_BW_15HZ63 0x09
#define BMX055_VAL_ACC_PMU_BW_31HZ25 0x0a
#define BMX055_VAL_ACC_PMU_BW_62HZ5 0x0b
#define BMX055_VAL_ACC_PMU_BW_125HZ 0x0c
#define BMX055_VAL_ACC_PMU_BW_250HZ 0x0d
#define BMX055_VAL_ACC_PMU_BW_500HZ 0x0e
#define BMX055_VAL_ACC_PMU_BW_1KHZ 0x0f
#define BMX055_REG_ACC_PMU_LPW 0x11
#define BMX055_VAL_ACC_PMU_LPW_NORMAL 0x00
#define BMX055_REG_ACC_PMU_LOW_POWER 0x12
#define BMX055_REG_ACC_ACCD_HBW 0x13
#define BMX055_VAL_ACC_ACCD_HBW_NORMAL 0x00 /* filtered data, shadowed */
#define BMX055_REG_ACC_BGW_SOFTRESET 0x14
#define BMX055_VAL_ACC_BGW_SOFTRESET_REQ 0xb6 /* value to trigger a reset */
					/* 3ms delay required afterwards */
#define BMX055_REG_ACC_INT_EN_0 0x16
#define BMX055_REG_ACC_INT_EN_1 0x17
#define BMX055_REG_ACC_INT_EN_2 0x18
#define BMX055_REG_ACC_INT_MAP_0 0x19
#define BMX055_REG_ACC_INT_MAP_1 0x1a
#define BMX055_REG_ACC_INT_MAP_2 0x1b
#define BMX055_REG_ACC_INT_SRC 0x1e
#define BMX055_REG_ACC_INT_OUT_CTRL 0x20
#define BMX055_REG_ACC_INT_RST_LATCH 0x21
#define BMX055_REG_ACC_INT_0 0x22
#define BMX055_REG_ACC_INT_1 0x23
#define BMX055_REG_ACC_INT_2 0x24
#define BMX055_REG_ACC_INT_3 0x25
#define BMX055_REG_ACC_INT_4 0x26
#define BMX055_REG_ACC_INT_5 0x27
#define BMX055_REG_ACC_INT_6 0x28
#define BMX055_REG_ACC_INT_7 0x29
#define BMX055_REG_ACC_INT_8 0x2a
#define BMX055_REG_ACC_INT_9 0x2b
#define BMX055_REG_ACC_INT_A 0x2c
#define BMX055_REG_ACC_INT_B 0x2d
#define BMX055_REG_ACC_INT_C 0x2e
#define BMX055_REG_ACC_INT_D 0x2f
#define BMX055_REG_ACC_FIFO_CONFIG_0 0x30
#define BMX055_REG_ACC_PMU_SELF_TEST 0x32
#define BMX055_REG_ACC_TRIM_NVM_CTRL 0x33
#define BMX055_REG_ACC_BGW_SPI3_WDT 0x34
#define BMX055_REG_ACC_OFC_CTRL 0x36
#define BMX055_REG_ACC_OFC_SETTING 0x37
#define BMX055_REG_ACC_OFC_OFFSET_X 0x38
#define BMX055_REG_ACC_OFC_OFFSET_Y 0x39
#define BMX055_REG_ACC_OFC_OFFSET_Z 0x3a
#define BMX055_REG_ACC_TRIM_GP0 0x3b
#define BMX055_REG_ACC_TRIM_GP1 0x3c
#define BMX055_REG_ACC_FIFO_CONFIG_1 0x3e
#define BMX055_VAL_ACC_FIFO_CONFIG_1_BYPASS 0x00
#define BMX055_REG_ACC_FIFO_DATA 0x3f

#define BMX055_REG_GYRO_CHIPID 0x00
#define BMX055_VAL_GYRO_CHIPID 0x0f
#define BMX055_REG_GYRO_X_LSB 0x02
#define BMX055_REG_GYRO_X_MSB 0x03
#define BMX055_REG_GYRO_Y_LSB 0x04
#define BMX055_REG_GYRO_Y_MSB 0x05
#define BMX055_REG_GYRO_Z_LSB 0x06
#define BMX055_REG_GYRO_Z_MSB 0x07
#define BMX055_REG_GYRO_INT_STATUS_0 0x09
#define BMX055_REG_GYRO_INT_STATUS_1 0x0A
#define BMX055_REG_GYRO_INT_STATUS_2 0x0B
#define BMX055_REG_GYRO_INT_STATUS_3 0x0C
#define BMX055_REG_GYRO_FIFO_STATUS 0x0E
#define BMX055_REG_GYRO_RANGE 0x0F
#define BMX055_VAL_GYRO_RANGE_2000DPS 0x00
#define BMX055_VAL_GYRO_RANGE_1000DPS 0x01
#define BMX055_VAL_GYRO_RANGE_500DPS 0x02
#define BMX055_VAL_GYRO_RANGE_250DPS 0x03
#define BMX055_VAL_GYRO_RANGE_125DPS 0x04
#define BMX055_REG_GYRO_BW 0x10
#define BMX055_VAL_GYRO_BW_47HZ 0x03	/* 400Hz ODR */
#define BMX055_VAL_GYRO_BW_116HZ 0x02	/* 1KHz ODR */
#define BMX055_VAL_GYRO_BW_230HZ 0x01	/* 2KHz ODR */
#define BMX055_VAL_GYRO_BW_UNFILT 0x00	/* 2KHz ODR */
#define BMX055_REG_GYRO_LPM1 0x11
#define BMX055_REG_GYRO_LPM2 0x12
#define BMX055_REG_GYRO_RATE_HBW 0x13
#define BMX055_REG_GYRO_BGW_SOFTRESET 0x14
#define BMX055_VAL_GYRO_BGW_SOFTRESET_REQ 0xb6	/* Takes 30ms! Typical! */
#define BMX055_REG_GYRO_INT_EN_0 0x15
#define BMX055_REG_GYRO_INT_EN_1 0x16
#define BMX055_REG_GYRO_INT_MAP_0 0x17
#define BMX055_REG_GYRO_INT_MAP_1 0x18
#define BMX055_REG_GYRO_INT_MAP_2 0x19
#define BMX055_REG_GYRO_INTERRUPTS_SELECTABLE_DATA_SOURCE 0x1A
#define BMX055_REG_GYRO_FAST_OFFSET_COMPENSATION_MOTION_THRESHOLD 0x1B
#define BMX055_REG_GYRO_MOTION_INT 0x1C
#define BMX055_REG_GYRO_FIFO_WM_INT 0x1E
#define BMX055_REG_GYRO_INT_RST_LATCH 0x21
#define BMX055_REG_GYRO_HIGH_TH_X 0x22
#define BMX055_REG_GYRO_HIGH_DUR_X 0x23
#define BMX055_REG_GYRO_HIGH_TH_Y 0x24
#define BMX055_REG_GYRO_HIGH_DUR_Y 0x25
#define BMX055_REG_GYRO_HIGH_TH_Z 0x26
#define BMX055_REG_GYRO_HIGH_DUR_Z 0x27
#define BMX055_REG_GYRO_SOC 0x31
#define BMX055_REG_GYRO_A_FOC 0x32
#define BMX055_REG_GYRO_TRIM_NVM_CTRL 0x33
#define BMX055_REG_GYRO_BGW_SPI3_WDT 0x34
#define BMX055_REG_GYRO_OFC1 0x36
#define BMX055_REG_GYRO_OFC2 0x37
#define BMX055_REG_GYRO_OFC3 0x38
#define BMX055_REG_GYRO_OFC4 0x39
#define BMX055_REG_GYRO_TRIM_GP0 0x3A
#define BMX055_REG_GYRO_TRIM_GP1 0x3B
#define BMX055_REG_GYRO_BIST 0x3C
#define BMX055_REG_GYRO_FIFO_CONFIG_0 0x3D
#define BMX055_REG_GYRO_FIFO_CONFIG_1 0x3E
#define BMX055_REG_GYRO_FIFO_DATA 0x3F

#endif /* PIOS_BMX055_PRIV_H */

/** 
  * @}
  * @}
  */

