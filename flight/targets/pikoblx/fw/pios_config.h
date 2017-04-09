/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup PikoBLX Tau Labs PikoBLX support files
 * @{
 *
 * @file       pios_config.h 
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @brief      Board specific options that modify PiOS capabilities
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

/* Enable/Disable PiOS Modules */
#define PIOS_INCLUDE_DMA_CB_SUBSCRIBING_FUNCTION

#define PIOS_INCLUDE_SPI
#define PIOS_INCLUDE_I2C
//#define PIOS_INCLUDE_GPIO

/* Select the sensors to include */
#define PIOS_INCLUDE_MS5611
#define PIOS_INCLUDE_MPU
#define PIOS_INCLUDE_HMC5883
#define PIOS_INCLUDE_HMC5983_I2C
#define PIOS_TOLERATE_MISSING_SENSORS

/* Com systems to include */
#define PIOS_INCLUDE_MAVLINK
#define PIOS_INCLUDE_LIGHTTELEMETRY 

/* Supported receiver interfaces */


/* Flags that alter behaviors - mostly to lower resources for CC */
//#define PIOS_TELEM_PRIORITY_QUEUE       /* Enable a priority queue in telemetry */
#define AUTOTUNE_AVERAGING_MODE
#define AUTOTUNE_AVERAGING_DECIMATION 2

/* Alarm Thresholds */

/* Task stack sizes */
#define PIOS_EVENTDISPATCHER_STACK_SIZE	1024

/*
 * This has been calibrated 2014/02/21 using chibios @ b89da8ac379646ac421bb65a209210e637bba223.
 * Calibration has been done by disabling the init task, breaking into debugger after
 * approximately after 60 seconds, then doing the following math:
 *
 * IDLE_COUNTS_PER_SEC_AT_NO_LOAD = (uint32_t)((double)idleCounter / xTickCount * 1000 + 0.5)
 *
 * This has to be redone every time the toolchain, toolchain flags or RTOS
 * configuration like number of task priorities or similar changes.
 * A change in the cpu load calculation or the idle task handler will invalidate this as well.
 */
#define IDLE_COUNTS_PER_SEC_AT_NO_LOAD (2175780)

#define CAMERASTAB_POI_MODE

#define PIOS_INCLUDE_FASTHEAP

#endif /* PIOS_CONFIG_H */
/**
 * @}
 * @}
 */
