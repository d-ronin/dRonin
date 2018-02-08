/**
******************************************************************************
* @addtogroup PIOS PIOS Core hardware abstraction layer
* @{
* @addtogroup PIOS_RFM22B Radio Functions
* @brief PIOS OpenLRS interface for for the RFM22B radio
* @{
*
* @file       pios_openlrs.h
* @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
* @author     dRonin, http://dronin.org Copyright (C) 2015
* @brief      Implements an OpenLRS driver for the RFM22B
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

#ifndef PIOS_OPENLRS_H
#define PIOS_OPENLRS_H

#include <hwshared.h>

struct pios_openlrs_cfg;

#ifdef PIOS_INCLUDE_OPENLRS
enum gpio_direction { GPIO0_TX_GPIO1_RX, GPIO0_RX_GPIO1_TX };

/* Global Types */
struct pios_openlrs_cfg {
	const struct pios_spi_cfg *spi_cfg; /* Pointer to SPI interface configuration */
	const struct pios_exti_cfg *exti_cfg; /* Pointer to the EXTI configuration */
	enum gpio_direction gpio_direction; /* Definition comes from pios_rfm22b.h */
};

struct pios_openlrs_dev;
typedef struct pios_openlrs_dev *pios_openlrs_t;

int32_t PIOS_OpenLRS_Init(pios_openlrs_t *openlrs_id, pios_spi_t spi_id,
		uint32_t slave_num, const struct pios_openlrs_cfg *cfg,
		HwSharedRfBandOptions rf_band);

int32_t PIOS_OpenLRS_Start(pios_openlrs_t openlrs_id);

void PIOS_OpenLRS_RegisterRcvr(pios_openlrs_t openlrs_id,
		uintptr_t rfm22b_rcvr_id);
uint8_t PIOS_OpenLRS_RSSI_Get(void);

bool PIOS_OpenLRS_EXT_Int(pios_openlrs_t openlrs_id);

#endif
#endif /* PIOS_OPENLRS_H */
/**
 * @}
 * @}
 */
