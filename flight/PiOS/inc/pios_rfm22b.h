/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_RFM22B Radio Functions
 * @brief PIOS interface for RFM22B Radio
 * @{
 *
 * @file       pios_rfm22b.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013 - 2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @brief      RFM22B functions header.
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

#ifndef PIOS_RFM22B_H
#define PIOS_RFM22B_H

#include <uavobjectmanager.h>
#include <hwshared.h>

/* Constant definitions */
enum gpio_direction { GPIO0_TX_GPIO1_RX, GPIO0_RX_GPIO1_TX };
#define RFM22B_PPM_NUM_CHANNELS 8

/* Global Types */
struct pios_rfm22b_cfg {
	const struct pios_spi_cfg *spi_cfg;	/* Pointer to SPI interface configuration */
	const struct pios_exti_cfg *exti_cfg;	/* Pointer to the EXTI configuration */
	uint8_t RFXtalCap;
	uint32_t slave_num;
	enum gpio_direction gpio_direction;
};

enum rfm22b_tx_power {
	RFM22_tx_pwr_txpow_0 = 0x00,	// +1dBm .. 1.25mW
	RFM22_tx_pwr_txpow_1 = 0x01,	// +2dBm .. 1.6mW
	RFM22_tx_pwr_txpow_2 = 0x02,	// +5dBm .. 3.16mW
	RFM22_tx_pwr_txpow_3 = 0x03,	// +8dBm .. 6.3mW
	RFM22_tx_pwr_txpow_4 = 0x04,	// +11dBm .. 12.6mW
	RFM22_tx_pwr_txpow_5 = 0x05,	// +14dBm .. 25mW
	RFM22_tx_pwr_txpow_6 = 0x06,	// +17dBm .. 50mW
	RFM22_tx_pwr_txpow_7 = 0x07	// +20dBm .. 100mW
};

typedef enum {
	PIOS_RFM22B_INT_FAILURE,
	PIOS_RFM22B_INT_SUCCESS,
	PIOS_RFM22B_TX_COMPLETE,
	PIOS_RFM22B_RX_COMPLETE
} pios_rfm22b_int_result;

struct rfm22b_stats {
	uint16_t tx_byte_count;
	uint16_t rx_byte_count;
	uint8_t rx_good;
	uint8_t rx_corrected;
	uint8_t rx_error;
	uint8_t rx_sync_missed;
	uint8_t rx_failure;
	uint8_t tx_missed;
	uint8_t resets;
	uint8_t timeouts;
	uint8_t link_quality;
	int8_t rssi;
	int8_t afc_correction;
	uint8_t link_state;
};

/* Public Functions */
extern int32_t PIOS_RFM22B_Init(uint32_t * rfb22b_id, uint32_t spi_id,
				uint32_t slave_num,
				const struct pios_rfm22b_cfg *cfg,
				HwSharedRfBandOptions band);
extern void PIOS_RFM22B_Reinit(uint32_t rfb22b_id);
extern void PIOS_RFM22B_SetTxPower(uint32_t rfm22b_id,
				   enum rfm22b_tx_power tx_pwr);
extern void PIOS_RFM22B_Config(uint32_t rfm22b_id,
					 HwSharedMaxRfSpeedOptions datarate,
					 uint8_t min_chan,
					 uint8_t max_chan,
					 uint32_t coordinator_id, bool oneway,
					 bool ppm_mode, bool ppm_only);
extern uint32_t PIOS_RFM22B_DeviceID(uint32_t rfb22b_id);
extern uint32_t PIOS_RFM22B_ModuleVersion(uint32_t rfb22b_id);
extern void PIOS_RFM22B_GetStats(uint32_t rfm22b_id,
				 struct rfm22b_stats *stats);
extern bool PIOS_RFM22B_LinkStatus(uint32_t rfm22b_id);
extern bool PIOS_RFM22B_ReceivePacket(uint32_t rfm22b_id, uint8_t * p);
extern bool PIOS_RFM22B_TransmitPacket(uint32_t rfm22b_id, uint8_t * p,
				       uint8_t len);
extern pios_rfm22b_int_result PIOS_RFM22B_ProcessTx(uint32_t rfm22b_id);
extern pios_rfm22b_int_result PIOS_RFM22B_ProcessRx(uint32_t rfm22b_id);
extern void PIOS_RFM22B_RegisterRcvr(uint32_t rfm22b_id, uintptr_t rfm22b_rcvr_id);
extern void PIOS_RFM22B_PPMSet(uint32_t rfm22b_id, int16_t * channels);
extern bool PIOS_RFM22B_IsCoordinator(uint32_t rfm22b_id);
extern uint8_t PIOS_RFM22B_RSSI_Get(void);

/* Global Variables */
extern const struct pios_com_driver pios_rfm22b_com_driver;

#endif /* PIOS_RFM22B_H */

/**
 * @}
 * @}
 */
