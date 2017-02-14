/**
 ******************************************************************************
 * @file       pios_usb_board_data.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @author     dRonin, http://dronin.org Copyright (C) 2016-2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_USB USB Functions
 * @{
 * @brief Defines for board specific usb information
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

#ifndef PIOS_USB_BOARD_DATA_H
#define PIOS_USB_BOARD_DATA_H

#include "pios_usb_defs.h" 	/* USB_* macros */

#define PIOS_USB_BOARD_CDC_DATA_LENGTH 64
#define PIOS_USB_BOARD_CDC_MGMT_LENGTH 32
#define PIOS_USB_BOARD_HID_DATA_LENGTH 64

#if defined(F1_UPGRADER) || defined(BOOTLOADER)

#	define PIOS_USB_BOARD_VENDOR_ID DRONIN_VID_DRONIN_BOOTLOADER
#	define PIOS_USB_BOARD_PRODUCT_ID DRONIN_PID_DRONIN_BOOTLOADER

	/*
	 * The bootloader uses a simplified report structure
	 *   BL: <REPORT_ID><DATA>...<DATA>
	 *   FW: <REPORT_ID><LENGTH><DATA>...<DATA>
	 * This define changes the behaviour in pios_usb_hid.c
	 */
#	define PIOS_USB_BOARD_BL_HID_HAS_NO_LENGTH_BYTE

#	define PIOS_USB_BOARD_EP_NUM 2

#	if defined(BOOTLOADER)
#		define PIOS_USB_BOARD_DEVICE_VER USB_OP_DEVICE_VER(0, USB_OP_BOARD_MODE_BL)
#		define PIOS_USB_BOARD_SN_SUFFIX "+BL"
#	elif defined (F1_UPGRADER)
#		define PIOS_USB_BOARD_DEVICE_VER USB_OP_DEVICE_VER(0, USB_OP_BOARD_MODE_UP)
#		define PIOS_USB_BOARD_SN_SUFFIX "+UP"
#	endif

#else  /* Firmware (...or not bootloader or f1 upgrader) */

#	define PIOS_USB_BOARD_VENDOR_ID DRONIN_VID_DRONIN_FIRMWARE
#	define PIOS_USB_BOARD_PRODUCT_ID DRONIN_PID_DRONIN_FIRMWARE

#	define PIOS_USB_BOARD_EP_NUM 4
#	define PIOS_USB_BOARD_DEVICE_VER USB_OP_DEVICE_VER(0, USB_OP_BOARD_MODE_FW)
#	define PIOS_USB_BOARD_SN_SUFFIX "+FW"

#endif /* defined(F1_UPGRADER) || defined(BOOTLOADER) */

#endif	/* PIOS_USB_BOARD_DATA_H */

/**
 * @}
 * @}
 */
