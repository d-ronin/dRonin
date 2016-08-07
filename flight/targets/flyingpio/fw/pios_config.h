/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup Sparky Tau Labs Sparky support files
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef PIOS_CONFIG_H
#define PIOS_CONFIG_H

#define PIOS_INCLUDE_SYS

/* Enable/Disable PiOS Modules */

/* Select the sensors to include */
//#define PIOS_INCLUDE_MS5611
//#define PIOS_INCLUDE_ETASV3
//#define PIOS_INCLUDE_MPXV5004
//#define PIOS_INCLUDE_MPXV7002

/* Com systems to include */
#define PIOS_INCLUDE_COM
#define PIOS_INCLUDE_LED
#define PIOS_INCLUDE_TIM
#define PIOS_INCLUDE_SERVO

/* Supported receiver interfaces */
//#define PIOS_INCLUDE_RCVR
//#define PIOS_INCLUDE_DSM
//#define PIOS_INCLUDE_HSUM
//#define PIOS_INCLUDE_SBUS
#define PIOS_INCLUDE_PPM
//#define PIOS_INCLUDE_SRXL
//#define PIOS_INCLUDE_IBUS

//#define PIOS_INCLUDE_FLASH
//#define PIOS_INCLUDE_FLASH_INTERNAL
//#define PIOS_INCLUDE_LOGFS_SETTINGS

/* Flags that alter behaviors - mostly to lower resources for CC */
//#define PIOS_INCLUDE_INITCALL           /* Include init call structures */
//#define PIOS_TELEM_PRIORITY_QUEUE       /* Enable a priority queue in telemetry */

//#define SUPPORTS_EXTERNAL_MAG

#endif /* PIOS_CONFIG_H */
/**
 * @}
 * @}
 */
