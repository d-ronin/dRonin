/**
 ******************************************************************************
 * @file       playuavosd/bl/pios_config.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup Bootloader Bootloaders
 * @{
 * @addtogroup PlayUAVOSD PlayUAV OSD
 * @{
 * @brief Bootloader for PlayUAVOSD board
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

/* Enable/Disable PiOS Modules */
#define PIOS_INCLUDE_DELAY
#define PIOS_INCLUDE_IRQ
#define PIOS_INCLUDE_ANNUNC
#define PIOS_INCLUDE_SYS
#define PIOS_INCLUDE_IAP
#define PIOS_INCLUDE_USB
#define PIOS_INCLUDE_USB_HID
#define PIOS_INCLUDE_COM_MSG
#define PIOS_INCLUDE_FLASH
#define PIOS_INCLUDE_FLASH_INTERNAL

#endif /* PIOS_CONFIG_H */

/**
 * @}
 * @}
 */
