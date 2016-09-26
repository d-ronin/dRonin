/**
 ******************************************************************************
 *
 * @file       pios_config.h 
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     dRonin, http://dRonin.org, Copyright (C) 2016
 * @addtogroup Targets
 * @{
 * @addtogroup FlyingPIO Programmed-IO expansion board target
 * @{
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

/* Systems to include */
#define PIOS_INCLUDE_ADC
#define PIOS_INCLUDE_COM
#define PIOS_INCLUDE_IRQ
#define PIOS_INCLUDE_ANNUNC
#define PIOS_INCLUDE_RTC	// Not really.. implemented by delay subsystem
#define PIOS_INCLUDE_SERVO
#define PIOS_INCLUDE_SPISLAVE
#define PIOS_INCLUDE_TIM
#define PIOS_INCLUDE_USART
#define PIOS_INCLUDE_WDG

#define PIOS_NO_ALARMS
#define PIOS_NO_MODULES

/* Supported receiver interfaces */
#define PIOS_INCLUDE_RCVR
#define PIOS_INCLUDE_DSM
#define PIOS_INCLUDE_HSUM
#define PIOS_INCLUDE_SBUS
#define PIOS_INCLUDE_PPM
#define PIOS_INCLUDE_SRXL
#define PIOS_INCLUDE_IBUS

#endif /* PIOS_CONFIG_H */
/**
 * @}
 * @}
 */
