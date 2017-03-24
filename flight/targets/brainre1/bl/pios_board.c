/**
 ******************************************************************************
 * @addtogroup Bootloader Bootloaders
 * @{
 * @addtogroup BrainRE1 BrainFPV RE1
 * @{
 *
 * @file       brainre1/bl/pios_board.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     BrainFPV, Copyright (C) 2015
 *
 * @brief      Board specific initialization for the bootloader
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

/* Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */
#include "board_hw_defs.c"

#include <pios_board_info.h>
#include <pios.h>

uintptr_t pios_com_telem_usb_id;

void PIOS_Board_Init() {

	/* Delay system */
	PIOS_DELAY_Init();
	
#if defined(PIOS_INCLUDE_ANNUNC)
	PIOS_ANNUNC_Init(&pios_annunc_cfg);
#endif	/* PIOS_INCLUDE_ANNUNC */

	PWR_BackupAccessCmd(ENABLE);
	RCC_LSEConfig(RCC_LSE_OFF);

#if defined(PIOS_INCLUDE_SPI)
	/* Set up the SPI interface to the flash */
	if (PIOS_SPI_Init(&pios_spi_flash_id, &pios_spi_flash_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
#endif	/* PIOS_INCLUDE_SPI */

#if defined(PIOS_INCLUDE_RE1_FPGA)
	if (PIOS_RE1FPGA_Init(pios_spi_flash_id, 1, &pios_re1fpga_cfg, true) != 0) {
		PIOS_DEBUG_Assert(0);
	}
	// For the bootloader, we use a green notification LED
	PIOS_RE1FPGA_SetNotificationLedColor(PIOS_RE1FPGA_STATUS_GREEN_CUSTOM_BLUE);
#endif /* defined(PIOS_INCLUDE_RE1_FPGA) */

#if defined(PIOS_INCLUDE_FLASH)
	/* Inititialize all flash drivers */
	PIOS_Flash_Internal_Init(&pios_internal_flash_id, &flash_internal_cfg);

#if defined(PIOS_INCLUDE_FLASH_JEDEC)
	PIOS_Flash_Jedec_Init(&pios_external_flash_id, pios_spi_flash_id, 0, &flash_s25fl127_cfg);
#endif	/* PIOS_INCLUDE_FLASH_JEDEC */

	/* Register the partition table */
	PIOS_FLASH_register_partition_table(pios_flash_partition_table, NELEMENTS(pios_flash_partition_table));

#endif	/* PIOS_INCLUDE_FLASH */

#if defined(PIOS_INCLUDE_USB)
	/* Initialize board specific USB data */
	PIOS_USB_BOARD_DATA_Init();

	/* Activate the HID-only USB configuration */
	PIOS_USB_DESC_HID_ONLY_Init();

	uintptr_t pios_usb_id;
	PIOS_USB_Init(&pios_usb_id, &pios_usb_main_cfg);

#if defined(PIOS_INCLUDE_USB_HID) && defined(PIOS_INCLUDE_COM_MSG)
	uintptr_t pios_usb_hid_id;
	if (PIOS_USB_HID_Init(&pios_usb_hid_id, &pios_usb_hid_cfg, pios_usb_id)) {
		PIOS_Assert(0);
	}
	if (PIOS_COM_MSG_Init(&pios_com_telem_usb_id, &pios_usb_hid_com_driver, pios_usb_hid_id)) {
		PIOS_Assert(0);
	}
#endif	/* PIOS_INCLUDE_USB_HID && PIOS_INCLUDE_COM_MSG */

	PIOS_USBHOOK_Activate();

#endif	/* PIOS_INCLUDE_USB */
}

/**
 * @}
 * @}
 */
