/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup BrainRE1 BrainFPV RE1
 * @{
 *
 * @file       brainre1/fw/pios_config.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef PIOS_CONFIG_H
#define PIOS_CONFIG_H

#include <pios_flight_config.h>

/* Major features */
#define STABILIZATION_LQG

/* Enable/Disable PiOS Modules */
#define PIOS_INCLUDE_FLASH_JEDEC
#define PIOS_INCLUDE_I2C
#define PIOS_INCLUDE_SPI
#define PIOS_INCLUDE_HPWM
#define PIOS_INCLUDE_TBSVTXCONFIG

/* Select the sensors to include */
#define PIOS_INCLUDE_BMI160
#define PIOS_INCLUDE_BMP280
#define PIOS_INCLUDE_MS5611
#define PIOS_INCLUDE_HMC5883
#define PIOS_INCLUDE_HMC5983_I2C
#define PIOS_TOLERATE_MISSING_SENSORS

#define FLASH_FREERTOS

/* Com systems to include */
#define PIOS_INCLUDE_MAVLINK
#define PIOS_INCLUDE_LIGHTTELEMETRY

#define PIOS_INCLUDE_DMASHOT

/* Supported receiver interfaces */


#define PIOS_INCLUDE_LOG_TO_FLASH
#define PIOS_LOGFLASH_SECT_SIZE 0x10000 /* 64kb */

#define PIOS_INCLUDE_IR_TRANSPONDER

/* RE1 specific drivers */
#define PIOS_INCLUDE_RE1_FPGA

/* OSD stuff */
#define PIOS_INCLUDE_VIDEO_QUADSPI
#define PIOS_VIDEO_BITS_PER_PIXEL 2
#define PIOS_VIDEO_QUADSPI_Y_OFFSET 3
#define MODULE_FLIGHTSTATS_BUILTIN
#define PIOS_INCLUDE_DEBUG_CONSOLE
#define OSD_USE_BRAINFPV_LOGO
#define OSD_USE_MENU

/* Flags that alter behaviors */
#define AUTOTUNE_AVERAGING_DECIMATION 2
#define SYSTEMMOD_RGBLED_SUPPORT

/* Alarm Thresholds */

/*
 * This has been calibrated 2014/03/01 using chibios @ fbd194c026098076bddd9e45e147828000f39d89
 * Calibration has been done by disabling the init task, breaking into debugger after
 * approximately after 60 seconds, then doing the following math:
 *
 * IDLE_COUNTS_PER_SEC_AT_NO_LOAD = (uint32_t)((double)idleCounter / xTickCount * 1000 + 0.5)
 *
 * This has to be redone every time the toolchain, toolchain flags or FreeRTOS
 * configuration like number of task priorities or similar changes.
 * A change in the cpu load calculation or the idle task handler will invalidate this as well.
 */
#define IDLE_COUNTS_PER_SEC_AT_NO_LOAD (9873737)

#define BRAIN

#endif /* PIOS_CONFIG_H */

/**
 * @}
 * @}
 */
