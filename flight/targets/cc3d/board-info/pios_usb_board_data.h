/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup CopterControl OpenPilot coptercontrol support files
 * @{
 *
 * @file       pios_usb_board_data.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @author     dRonin, http://dronin.org Copyright (C) 2016
 * @brief      Defines for board specific usb information
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

#ifndef PIOS_USB_BOARD_DATA_H
#define PIOS_USB_BOARD_DATA_H

#define PIOS_USB_BOARD_CDC_DATA_LENGTH 64
#define PIOS_USB_BOARD_CDC_MGMT_LENGTH 32
#define PIOS_USB_BOARD_HID_DATA_LENGTH 64


#include "pios_usb_defs.h" 	/* USB_* macros */

#define PIOS_USB_BOARD_VENDOR_ID USB_VENDOR_ID_OPENPILOT
#define PIOS_USB_BOARD_PRODUCT_ID USB_PRODUCT_ID_CC3D

#if defined(F1_UPGRADER)

#define PIOS_USB_BOARD_EP_NUM 2
#define PIOS_USB_BOARD_DEVICE_VER USB_OP_DEVICE_VER(0, USB_OP_BOARD_MODE_UP)
#define PIOS_USB_BOARD_SN_SUFFIX "+UP"

/*
 * The bootloader uses a simplified report structure
 *   BL: <REPORT_ID><DATA>...<DATA>
 *   FW: <REPORT_ID><LENGTH><DATA>...<DATA>
 * This define changes the behaviour in pios_usb_hid.c
 */
#define PIOS_USB_BOARD_BL_HID_HAS_NO_LENGTH_BYTE

#elif defined(BOOTLOADER)

#define PIOS_USB_BOARD_EP_NUM 2
#define PIOS_USB_BOARD_DEVICE_VER USB_OP_DEVICE_VER(0, USB_OP_BOARD_MODE_BL)
#define PIOS_USB_BOARD_SN_SUFFIX "+BL"

/*
 * The bootloader uses a simplified report structure
 *   BL: <REPORT_ID><DATA>...<DATA>
 *   FW: <REPORT_ID><LENGTH><DATA>...<DATA>
 * This define changes the behaviour in pios_usb_hid.c
 */
#define PIOS_USB_BOARD_BL_HID_HAS_NO_LENGTH_BYTE

#else  /* Not bootloader or f1 upgrader */

#define PIOS_USB_BOARD_EP_NUM 4
#define PIOS_USB_BOARD_DEVICE_VER USB_OP_DEVICE_VER(0, USB_OP_BOARD_MODE_FW)
#define PIOS_USB_BOARD_SN_SUFFIX "+FW"

#endif

#endif	/* PIOS_USB_BOARD_DATA_H */
