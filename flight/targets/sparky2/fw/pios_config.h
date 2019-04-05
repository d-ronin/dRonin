/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup Sparky2 Tau Labs Sparky2
 * @{
 *
 * @file       sparky2/fw/pios_config.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @brief      Board configuration file
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


#ifndef PIOS_CONFIG_H
#define PIOS_CONFIG_H

#include <pios_flight_config.h>

/* Major features */
#define PIOS_INCLUDE_FLASH_JEDEC
#define STABILIZATION_LQG

/* Enable/Disable PiOS Modules */
#define PIOS_INCLUDE_I2C
#define PIOS_INCLUDE_SPI
#define PIOS_INCLUDE_CAN
#define PIOS_INCLUDE_FASTHEAP

/* Variables related to the RFM22B functionality */
#define PIOS_INCLUDE_OPENLRS
#define PIOS_INCLUDE_OPENLRS_RCVR

/* Select the sensors to include */
#define PIOS_INCLUDE_MPU
#define PIOS_INCLUDE_MPU_MAG
#define PIOS_INCLUDE_MS5611
//#define PIOS_INCLUDE_ETASV3
#define PIOS_INCLUDE_MPXV5004
#define PIOS_INCLUDE_MPXV7002
#define PIOS_INCLUDE_HMC5883
#define PIOS_INCLUDE_HMC5983_I2C
//#define PIOS_INCLUDE_HCSR04

/* Com systems to include */
#define PIOS_INCLUDE_MAVLINK
//#define PIOS_INCLUDE_LIGHTTELEMETRY

/* Supported receiver interfaces */
#define PIOS_INCLUDE_PWM


#define PIOS_INCLUDE_DEBUG_CONSOLE

/* Flags that alter behaviors */

/* Alarm Thresholds */

/*
 * This has been calibrated 2013/03/11 using next @ 6d21c7a590619ebbc074e60cab5e134e65c9d32b.
 * Calibration has been done by disabling the init task, breaking into debugger after
 * approximately after 60 seconds, then doing the following math:
 *
 * IDLE_COUNTS_PER_SEC_AT_NO_LOAD = (uint32_t)((double)idleCounter / xTickCount * 1000 + 0.5)
 *
 * This has to be redone every time the toolchain, toolchain flags or RTOS
 * configuration like number of task priorities or similar changes.
 * A change in the cpu load calculation or the idle task handler will invalidate this as well.
 */
#define IDLE_COUNTS_PER_SEC_AT_NO_LOAD (9873737)

#define PIOS_INCLUDE_LOG_TO_FLASH
#define PIOS_LOGFLASH_SECT_SIZE 0x10000   /* 64kb */

#endif /* PIOS_CONFIG_H */

/**
 * @}
 * @}
 */
