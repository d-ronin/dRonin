/**
 ******************************************************************************
 *
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_FLASH Flash device handler
 * @{
 *
 * @file       pios_flash_jedec_priv.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Driver for talking to most JEDEC flash chips
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

#ifndef PIOS_FLASH_JEDEC_H_
#define PIOS_FLASH_JEDEC_H_

#include "pios_flash.h"		/* API definition for flash drivers */

extern const struct pios_flash_driver pios_jedec_flash_driver;

#define JEDEC_MANUFACTURER_ANY      0x00
#define JEDEC_MANUFACTURER_ST       0x20
#define JEDEC_MANUFACTURER_MACRONIX 0xC2
#define JEDEC_MANUFACTURER_WINBOND  0xEF
#define JEDEC_MANUFACTURER_SPANSION 0x01
#define JEDEC_MANUFACTURER_MICRON   0x20

#define JEDEC_MEMORYTYPE_ANY        0x00

struct pios_flash_jedec_cfg {
	uint8_t expect_manufacturer;
	uint8_t expect_memorytype;
	uint8_t expect_capacity;
	uint32_t sector_erase;
};

int32_t PIOS_Flash_Jedec_Init(uintptr_t *flash_id, pios_spi_t spi_id, uint32_t slave_num, const struct pios_flash_jedec_cfg *cfg);
int32_t PIOS_Flash_Jedec_ReadOTPData(uintptr_t chip_id, uint32_t chip_offset, uint8_t *data, uint16_t len);
uint32_t PIOS_Flash_Jedec_GetCapacity(uintptr_t chip_id);

#endif	/* PIOS_FLASH_JEDEC_H_ */
