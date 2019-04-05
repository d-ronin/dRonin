/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup RevoMini OpenPilot Revolution Mini
 * @{
 *
 * @file       revolution/fw/pios_config.h
 *
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
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
#define PIOS_INCLUDE_FASTHEAP
#define PIOS_INCLUDE_WS2811

/* Variables related to the RFM22B functionality */
#define PIOS_INCLUDE_OPENLRS
#define PIOS_INCLUDE_OPENLRS_RCVR

/* Or the OSD on clones */
#define PIOS_INCLUDE_MAX7456

/* Select the sensors to include */
#define PIOS_INCLUDE_BMP280
#define PIOS_INCLUDE_BMP280_SPI
#define PIOS_INCLUDE_HMC5883
#define PIOS_INCLUDE_HMC5983_I2C
#define PIOS_INCLUDE_MPU
#define PIOS_INCLUDE_MS5611
//#define PIOS_INCLUDE_ETASV3
#define PIOS_INCLUDE_MPXV5004
#define PIOS_INCLUDE_MPXV7002
//#define PIOS_INCLUDE_HCSR04
#define PIOS_TOLERATE_MISSING_SENSORS

/* Com systems to include */
#define PIOS_INCLUDE_MAVLINK
#define PIOS_INCLUDE_LIGHTTELEMETRY

/* Supported receiver interfaces */
#define PIOS_INCLUDE_PWM

//#define PIOS_INCLUDE_DEBUG_CONSOLE

/* Flags that alter behaviors */

#define SYSTEMMOD_RGBLED_SUPPORT

/* Alarm Thresholds */

/*
 * This has been calibrated 2015/12/06 using next @ f7f5c5ffe0a2b2fb9da819e32719f6b3bddacf86.
 * Calibration has been done by disabling the init task, breaking into debugger after
 * approximately after 60 seconds, then doing the following math:
 *
 * IDLE_COUNTS_PER_SEC_AT_NO_LOAD = (uint32_t)((double)idleCounter / vtlist.vt_systime * 1000 + 0.5)
 *
 * This has to be redone every time the toolchain, toolchain flags or RTOS
 * configuration like number of task priorities or similar changes.
 * A change in the cpu load calculation or the idle task handler will invalidate this as well.
 */
#define IDLE_COUNTS_PER_SEC_AT_NO_LOAD (9870518)

#define PIOS_INCLUDE_LOG_TO_FLASH
#define PIOS_LOGFLASH_SECT_SIZE 0x10000   /* 64kb */

#endif /* PIOS_CONFIG_H */

/**
 * @}
 * @}
 */
