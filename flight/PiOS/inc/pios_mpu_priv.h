/**
 ******************************************************************************
 * @file       pios_mpu_priv.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MPU Invensense MPU Functions
 * @{
 * @brief Common driver for Invensense MPU-xxx0 6/9-axis IMUs.
 * Private Declarations
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

#ifndef PIOS_MPU_PRIV_H
#define PIOS_MPU_PRIV_H

#include "pios_mpu.h"

/* MPU60X0 Addresses */
#define PIOS_MPU_PRODUCT_ID           0x0C
#define PIOS_MPU_SMPLRT_DIV_REG       0X19
#define PIOS_MPU_DLPF_CFG_REG         0X1A
#define PIOS_MPU_GYRO_CFG_REG         0X1B
#define PIOS_MPU_ACCEL_CFG_REG        0X1C
#define PIOS_MPU_ACCEL_CFG2_REG       0X1D
#define PIOS_MPU_FIFO_EN_REG          0x23
#define PIOS_MPU_SLV0_ADDR_REG        0x25
#define PIOS_MPU_SLV0_REG_REG         0x26
#define PIOS_MPU_SLV0_CTRL_REG        0x27
#define PIOS_MPU_SLV4_ADDR_REG        0x31
#define PIOS_MPU_SLV4_REG_REG         0x32
#define PIOS_MPU_SLV4_DO_REG          0x33
#define PIOS_MPU_SLV4_CTRL_REG        0x34
#define PIOS_MPU_SLV4_DI_REG          0x35
#define PIOS_MPU_I2C_MST_STATUS_REG   0x36
#define PIOS_MPU_INT_CFG_REG          0x37
#define PIOS_MPU_INT_EN_REG           0x38
#define PIOS_MPU_INT_STATUS_REG       0x3A
#define PIOS_MPU_ACCEL_X_OUT_MSB      0x3B
#define PIOS_MPU_ACCEL_X_OUT_LSB      0x3C
#define PIOS_MPU_ACCEL_Y_OUT_MSB      0x3D
#define PIOS_MPU_ACCEL_Y_OUT_LSB      0x3E
#define PIOS_MPU_ACCEL_Z_OUT_MSB      0x3F
#define PIOS_MPU_ACCEL_Z_OUT_LSB      0x40
#define PIOS_MPU_TEMP_OUT_MSB         0x41
#define PIOS_MPU_TEMP_OUT_LSB         0x42
#define PIOS_MPU_GYRO_X_OUT_MSB       0x43
#define PIOS_MPU_GYRO_X_OUT_LSB       0x44
#define PIOS_MPU_GYRO_Y_OUT_MSB       0x45
#define PIOS_MPU_GYRO_Y_OUT_LSB       0x46
#define PIOS_MPU_GYRO_Z_OUT_MSB       0x47
#define PIOS_MPU_GYRO_Z_OUT_LSB       0x48
#define PIOS_MPU_SIGNAL_PATH_RESET    0x68
#define PIOS_MPU_USER_CTRL_REG        0x6A
#define PIOS_MPU_PWR_MGMT_REG         0x6B
#define PIOS_MPU_FIFO_CNT_MSB         0x72
#define PIOS_MPU_FIFO_CNT_LSB         0x73
#define PIOS_MPU_FIFO_REG             0x74
#define PIOS_MPU_WHOAMI               0x75

/* FIFO enable for storing different values */
#define PIOS_MPU_FIFO_TEMP_OUT        0x80
#define PIOS_MPU_FIFO_GYRO_X_OUT      0x40
#define PIOS_MPU_FIFO_GYRO_Y_OUT      0x20
#define PIOS_MPU_FIFO_GYRO_Z_OUT      0x10
#define PIOS_MPU_ACCEL_OUT            0x08

/* Interrupt Configuration */
#define PIOS_MPU_INT_ACTL             0x80
#define PIOS_MPU_INT_OPEN             0x40
#define PIOS_MPU_INT_LATCH_EN         0x20
#define PIOS_MPU_INT_CLR_ANYRD        0x10
#define PIOS_MPU_INT_I2C_BYPASS_EN    0x02	

#define PIOS_MPU_INTEN_OVERFLOW       0x10
#define PIOS_MPU_INTEN_DATA_RDY       0x01

/* Interrupt status */
#define PIOS_MPU_INT_STATUS_OVERFLOW  0x10
#define PIOS_MPU_INT_STATUS_IMU_RDY   0X04
#define PIOS_MPU_INT_STATUS_DATA_RDY  0X01

/* User control functionality */
#define PIOS_MPU_USERCTL_FIFO_EN      0X40
#define PIOS_MPU_USERCTL_I2C_MST_EN   0X20
#define PIOS_MPU_USERCTL_DIS_I2C      0X10
#define PIOS_MPU_USERCTL_FIFO_RST     0X02
#define PIOS_MPU_USERCTL_GYRO_RST     0X01

/* Power management and clock selection */
#define PIOS_MPU_PWRMGMT_IMU_RST      0X80
#define PIOS_MPU_PWRMGMT_INTERN_CLK   0X00
#define PIOS_MPU_PWRMGMT_PLL_X_CLK    0X01
#define PIOS_MPU_PWRMGMT_PLL_Y_CLK    0X02
#define PIOS_MPU_PWRMGMT_PLL_Z_CLK    0X03
#define PIOS_MPU_PWRMGMT_STOP_CLK     0X07

/* I2C master status register bits */
#define PIOS_MPU_I2C_MST_SLV4_DONE    0x40
#define PIOS_MPU_I2C_MST_LOST_ARB     0x20
#define PIOS_MPU_I2C_MST_SLV4_NACK    0x10
#define PIOS_MPU_I2C_MST_SLV0_NACK    0x01

/* I2C SLV register bits */
#define PIOS_MPU_I2CSLV_EN            0x80
#define PIOS_MPU_I2CSLV_BYTE_SW       0x40
#define PIOS_MPU_I2CSLV_REG_DIS       0x20
#define PIOS_MPU_I2CSLV_GRP           0x10


/* AK89XX Registers */
#define PIOS_MPU_AK89XX_ADDR                         0x0C
#define PIOS_MPU_AK89XX_WHOAMI_REG                   0x00
#define PIOS_MPU_AK89XX_WHOAMI_ID                    0x48
#define PIOS_MPU_AK89XX_ST1_REG                      0x02
#define PIOS_MPU_AK89XX_ST2_REG                      0x09
#define PIOS_MPU_AK89XX_ST1_DRDY                     0x01
#define PIOS_MPU_AK8963_ST2_BITM                     0x10
#define PIOS_MPU_AK89XX_ST2_HOFL                     0x08
#define PIOS_MPU_AK8975_ST2_DERR                     0x04
#define PIOS_MPU_AK89XX_CNTL1_REG                    0x0A
#define PIOS_MPU_AK8963_CNTL2_REG                    0x0B
#define PIOS_MPU_AK8963_CNTL2_SRST                   0x01
#define PIOS_MPU_AK8963_MODE_CONTINUOUS_FAST_16B     0x16
#define PIOS_MPU_AK8975_MODE_SINGLE_12B              0x01

#endif /* PIOS_MPU_PRIV_H */

/** 
  * @}
  * @}
  */

