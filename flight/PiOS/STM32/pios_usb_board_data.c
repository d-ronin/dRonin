/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_USB_DESC USB Descriptor definitions
 * @{
 *
 * @file       PiOS/STM32/pios_usb_board_data.c
 * @author     dRonin, http://dronin.org, Copyright (C) 2017
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      Board specific USB strings
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

#include "pios_usb_board_data.h" /* struct usb_*, USB_* */
#include "pios_sys.h"		 /* PIOS_SYS_SerialNumberGet */
#include "pios_usbhook.h"	 /* PIOS_USBHOOK_* */
#include "pios_usb_util.h"	 /* PIOS_USB_UTIL_AsciiToUtf8 */

#define U16_CHARSEQ(val) val, 0,

#define U16_CHARS1(val) U16_CHARSEQ(val)
#define U16_CHARS2(val, ...) U16_CHARSEQ(val) U16_CHARS1(__VA_ARGS__)
#define U16_CHARS3(val, ...) U16_CHARSEQ(val) U16_CHARS2(__VA_ARGS__)
#define U16_CHARS4(val, ...) U16_CHARSEQ(val) U16_CHARS3(__VA_ARGS__)
#define U16_CHARS5(val, ...) U16_CHARSEQ(val) U16_CHARS4(__VA_ARGS__)
#define U16_CHARS6(val, ...) U16_CHARSEQ(val) U16_CHARS5(__VA_ARGS__)
#define U16_CHARS7(val, ...) U16_CHARSEQ(val) U16_CHARS6(__VA_ARGS__)
#define U16_CHARS8(val, ...) U16_CHARSEQ(val) U16_CHARS7(__VA_ARGS__)
#define U16_CHARS9(val, ...) U16_CHARSEQ(val) U16_CHARS8(__VA_ARGS__)
#define U16_CHARS10(val, ...) U16_CHARSEQ(val) U16_CHARS9(__VA_ARGS__)
#define U16_CHARS11(val, ...) U16_CHARSEQ(val) U16_CHARS10(__VA_ARGS__)
#define U16_CHARS12(val, ...) U16_CHARSEQ(val) U16_CHARS11(__VA_ARGS__)
#define U16_CHARS13(val, ...) U16_CHARSEQ(val) U16_CHARS12(__VA_ARGS__)
#define U16_CHARS14(val, ...) U16_CHARSEQ(val) U16_CHARS13(__VA_ARGS__)
#define U16_CHARS15(val, ...) U16_CHARSEQ(val) U16_CHARS14(__VA_ARGS__)
#define U16_CHARS16(val, ...) U16_CHARSEQ(val) U16_CHARS15(__VA_ARGS__)
#define U16_CHARS17(val, ...) U16_CHARSEQ(val) U16_CHARS16(__VA_ARGS__)

#define GENERATE_USB_STRING_IMPL(name, n, ...) static const uint8_t name[n*2+2] = { n*2+2, USB_DESC_TYPE_STRING, U16_CHARS##n(__VA_ARGS__) }
#define GENERATE_USB_STRING(name, n, v) GENERATE_USB_STRING_IMPL(name, n, v)

GENERATE_USB_STRING(usb_product_id, USB_STR_PROD_LEN, USB_STR_PROD_VAL);
GENERATE_USB_STRING(usb_vendor_id, USB_STR_VEND_LEN, USB_STR_VEND_VAL);

static uint8_t usb_serial_number[2 + PIOS_SYS_SERIAL_NUM_ASCII_LEN*2 + (sizeof(PIOS_USB_BOARD_SN_SUFFIX)-1)*2] = {
	sizeof(usb_serial_number),
	USB_DESC_TYPE_STRING,
};

static const struct usb_string_langid usb_lang_id = {
	.bLength = sizeof(usb_lang_id),
	.bDescriptorType = USB_DESC_TYPE_STRING,
	.bLangID = htousbs(USB_LANGID_ENGLISH_US),
};

int32_t PIOS_USB_BOARD_DATA_Init(void)
{
	/* Load device serial number into serial number string */
	uint8_t sn[PIOS_SYS_SERIAL_NUM_ASCII_LEN + 1];
	PIOS_SYS_SerialNumberGet((char *)sn);

	/* Concatenate the device serial number and the appropriate suffix ("+BL" or "+FW") into the USB serial number */
	uint8_t * utf8 = &(usb_serial_number[2]);
	utf8 = PIOS_USB_UTIL_AsciiToUtf8(utf8, sn, PIOS_SYS_SERIAL_NUM_ASCII_LEN);
	utf8 = PIOS_USB_UTIL_AsciiToUtf8(utf8, (uint8_t *)PIOS_USB_BOARD_SN_SUFFIX, sizeof(PIOS_USB_BOARD_SN_SUFFIX)-1);

	PIOS_USBHOOK_RegisterString(USB_STRING_DESC_PRODUCT, (uint8_t *)&usb_product_id, sizeof(usb_product_id));
	PIOS_USBHOOK_RegisterString(USB_STRING_DESC_SERIAL, (uint8_t *)&usb_serial_number, sizeof(usb_serial_number));

	PIOS_USBHOOK_RegisterString(USB_STRING_DESC_LANG, (uint8_t *)&usb_lang_id, sizeof(usb_lang_id));
	PIOS_USBHOOK_RegisterString(USB_STRING_DESC_VENDOR, (uint8_t *)&usb_vendor_id, sizeof(usb_vendor_id));

	return 0;
}

/**
 * @}
 * @}
 */
