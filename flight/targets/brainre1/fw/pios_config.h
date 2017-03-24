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

/* Major features */
#define PIOS_INCLUDE_CHIBIOS
#define PIOS_INCLUDE_BL_HELPER

/* Enable/Disable PiOS Modules */
#define PIOS_INCLUDE_ADC
#define PIOS_INCLUDE_I2C
#define WDG_STATS_DIAGNOSTICS
#define PIOS_INCLUDE_ANNUNC
#define PIOS_INCLUDE_IAP
#define PIOS_INCLUDE_TIM
#define PIOS_INCLUDE_SERVO
#define PIOS_INCLUDE_SPI
#define PIOS_INCLUDE_SYS
#define PIOS_INCLUDE_USART
#define PIOS_INCLUDE_USB
#define PIOS_INCLUDE_USB_HID
#define PIOS_INCLUDE_USB_CDC
//#define PIOS_INCLUDE_GPIO
#define PIOS_INCLUDE_EXTI
#define PIOS_INCLUDE_RTC
#define PIOS_INCLUDE_WDG
#define PIOS_INCLUDE_HPWM
#define PIOS_INCLUDE_FRSKY_RSSI
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
#define PIOS_INCLUDE_COM
#define PIOS_INCLUDE_COM_TELEM
#define PIOS_INCLUDE_TELEMETRY_RF
#define PIOS_INCLUDE_COM_FLEXI
#define PIOS_INCLUDE_MAVLINK
#define PIOS_INCLUDE_MSP_BRIDGE
#define PIOS_INCLUDE_HOTT
#define PIOS_INCLUDE_FRSKY_SENSOR_HUB
#define PIOS_INCLUDE_SESSION_MANAGEMENT
#define PIOS_INCLUDE_LIGHTTELEMETRY
#define PIOS_INCLUDE_FRSKY_SPORT_TELEMETRY
#define PIOS_INCLUDE_OPENLOG
#define PIOS_INCLUDE_STORM32BGC

#define PIOS_INCLUDE_GPS
#define PIOS_INCLUDE_GPS_NMEA_PARSER
#define PIOS_INCLUDE_GPS_UBX_PARSER
#define PIOS_GPS_SETS_HOMELOCATION

/* Supported receiver interfaces */
#define PIOS_INCLUDE_RCVR
#define PIOS_INCLUDE_DSM
#define PIOS_INCLUDE_SBUS
#define PIOS_INCLUDE_PPM
#define PIOS_INCLUDE_HSUM
#define PIOS_INCLUDE_SRXL
#define PIOS_INCLUDE_GCSRCVR
#define PIOS_INCLUDE_IBUS

#define PIOS_INCLUDE_FLASH
#define PIOS_INCLUDE_LOGFS_SETTINGS
#define PIOS_INCLUDE_FLASH_JEDEC
#define PIOS_INCLUDE_FLASH_INTERNAL

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

/* Flags that alter behaviors - mostly to lower resources for CC */
#define PIOS_INCLUDE_INITCALL           /* Include init call structures */
#define PIOS_TELEM_PRIORITY_QUEUE       /* Enable a priority queue in telemetry */
//#define PIOS_QUATERNION_STABILIZATION   /* Stabilization options */
#define PIOS_GPS_SETS_HOMELOCATION      /* GPS options */
#define AUTOTUNE_AVERAGING_MODE
#define AUTOTUNE_AVERAGING_DECIMATION 2
#define SYSTEMMOD_RGBLED_SUPPORT

#define CAMERASTAB_POI_MODE

/* Alarm Thresholds */
#define HEAP_LIMIT_WARNING		1000
#define HEAP_LIMIT_CRITICAL		500
#define IRQSTACK_LIMIT_WARNING		150
#define IRQSTACK_LIMIT_CRITICAL		80
#define CPULOAD_LIMIT_WARNING		80
#define CPULOAD_LIMIT_CRITICAL		95

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
