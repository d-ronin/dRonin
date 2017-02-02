/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_USB_UTIL USB utility functions
 * @brief USB utility functions
 * @{
 *
 * @file       pios_usb_util.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      USB utility functions
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

#ifndef PIOS_USB_UTIL_H
#define PIOS_USB_UTIL_H

#include <stdint.h>		/* uint8_t */

uint8_t * PIOS_USB_UTIL_AsciiToUtf8(uint8_t * dst, uint8_t * src, uint16_t srclen);

#endif	/* PIOS_USB_UTIL_H */
