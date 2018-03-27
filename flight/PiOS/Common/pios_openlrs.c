/**
******************************************************************************
* @addtogroup PIOS PIOS Core hardware abstraction layer
* @{
* @addtogroup	PIOS_RFM22B Radio Functions
* @brief PIOS OpenLRS interface for for the RFM22B radio
* @{
*
* @file       pios_openlrs.c
* @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
* @author     dRonin, http://dronin.org Copyright (C) 2015-2018
* @brief      Implements an OpenLRS driver for the RFM22B
* @see	      The GNU Public License (GPL) Version 3
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

#include "pios.h"

#ifdef PIOS_INCLUDE_OPENLRS

#include <pios_thread.h>
#include <pios_spi_priv.h>
#include <pios_openlrs_priv.h>
#include <pios_openlrs_rcvr_priv.h>
#include <taskmonitor.h>
#include <taskinfo.h>

#include "openlrs.h"
#include "flightstatus.h"
#include "flightbatterystate.h"
#include "manualcontrolsettings.h"
#include "openlrsstatus.h"

#include <pios_hal.h>	/* for rcvr_group_map in tx path */

#include "pios_rfm22b_regs.h"

#define STACK_SIZE_BYTES                 900
#define TASK_PRIORITY                    PIOS_THREAD_PRIO_HIGH

#define RF22B_PWRSTATE_POWERDOWN    0x00
#define RF22B_PWRSTATE_READY        (RFM22_opfc1_xton | RFM22_opfc1_pllon)
#define RF22B_PWRSTATE_RX           (RFM22_opfc1_rxon | RFM22_opfc1_xton)
#define RF22B_PWRSTATE_TX           (RFM22_opfc1_txon | RFM22_opfc1_xton)

static OpenLRSStatusData openlrs_status;

static pios_openlrs_t pios_openlrs_alloc();
static bool pios_openlrs_validate(pios_openlrs_t openlrs_dev);
static void pios_openlrs_rx_task(void *parameters);
static void pios_openlrs_tx_task(void *parameters);
static void pios_openlrs_do_hop(pios_openlrs_t openlrs_dev);

static void rfm22_init(pios_openlrs_t openlrs_dev, uint8_t isbind);
static void rfm22_disable(pios_openlrs_t openlrs_dev);
static void rfm22_set_frequency(pios_openlrs_t openlrs_dev, uint32_t f);
static uint8_t rfm22_get_rssi(pios_openlrs_t openlrs_dev);
static void rfm22_tx_packet(pios_openlrs_t openlrs_dev,
		void *pkt, uint8_t size);
static bool rfm22_rx_packet(pios_openlrs_t openlrs_dev,
		void *whence, uint32_t size);
static int rfm22_rx_packet_variable(pios_openlrs_t openlrs_dev,
		void *whence, uint32_t max_size);
static void rfm22_set_channel(pios_openlrs_t openlrs_dev, uint8_t ch);
static void rfm22_rx_reset(pios_openlrs_t openlrs_dev, bool pause_long);
static void rfm22_check_hang(pios_openlrs_t openlrs_dev);

// SPI read/write functions
static void rfm22_assert_cs(pios_openlrs_t openlrs_dev);
static void rfm22_deassert_cs(pios_openlrs_t openlrs_dev);
static void rfm22_claim_bus(pios_openlrs_t openlrs_dev);
static void rfm22_release_bus(pios_openlrs_t openlrs_dev);
static void rfm22_write_claim(pios_openlrs_t openlrs_dev,
		uint8_t addr, uint8_t data);
static void rfm22_write(pios_openlrs_t openlrs_dev, uint8_t addr,
		uint8_t data);
static uint8_t rfm22_read_claim(pios_openlrs_t openlrs_dev,
		uint8_t addr);
static uint8_t rfm22_read(pios_openlrs_t openlrs_dev,
		uint8_t addr);
static void rfm22_get_it_status(pios_openlrs_t openlrs_dev);
static void rfm22_beacon_send(pios_openlrs_t openlrs_dev, bool static_tone);

// Private constants
const struct rfm22_modem_regs {
	uint32_t bps;
	uint8_t r_1c, r_1d, r_1e, r_20, r_21, r_22, r_23, r_24, r_25, r_2a, r_6e, r_6f, r_70, r_71, r_72;
} modem_params[] = {
	{ 4800, 0x1a, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x1b, 0x1e, 0x27, 0x52, 0x2c, 0x23, 0x30 }, // 50000 0x00
	{ 9600, 0x05, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x20, 0x24, 0x4e, 0xa5, 0x2c, 0x23, 0x30 }, // 25000 0x00
	{ 19200, 0x06, 0x40, 0x0a, 0xd0, 0x00, 0x9d, 0x49, 0x00, 0x7b, 0x28, 0x9d, 0x49, 0x2c, 0x23, 0x30 }, // 25000 0x01
	{ 57600, 0x05, 0x40, 0x0a, 0x45, 0x01, 0xd7, 0xdc, 0x03, 0xb8, 0x1e, 0x0e, 0xbf, 0x00, 0x23, 0x2e },
	{ 125000, 0x8a, 0x40, 0x0a, 0x60, 0x01, 0x55, 0x55, 0x02, 0xad, 0x1e, 0x20, 0x00, 0x00, 0x23, 0xc8 },
};

/* invariants
 * 1D = 0x40 AFC enabled, gearshift = 000000
 * 20 = 0x0a clock recovery oversampling rate = 0x0a, min value 0x08
 * 71 = 0x23 modulation mode -- FIFO, GFSK
 */

#define OPENLRS_BIND_PARAMS 1	/* 9600 params */

static uint8_t count_set_bits(uint16_t x)
{
	return __builtin_popcount(x);
}

static bool wait_interrupt(pios_openlrs_t openlrs_dev, int delay_us)
{
	int delay_ms = (delay_us) / 1000;

	if (delay_ms <= 0) {
		delay_ms = 1;
	}

	return PIOS_Semaphore_Take(openlrs_dev->sema_isr, delay_ms);
}

static void olrs_bind_rx(uintptr_t olrs_id, pios_com_callback rx_in_cb,
		uintptr_t context)
{
	pios_openlrs_t openlrs_dev = (pios_openlrs_t) olrs_id;

	PIOS_Assert(pios_openlrs_validate(openlrs_dev));

	openlrs_dev->rx_in_cb = rx_in_cb;
	openlrs_dev->rx_in_context = context;
}

static void olrs_bind_tx(uintptr_t olrs_id, pios_com_callback tx_out_cb,
		uintptr_t context)
{
	pios_openlrs_t openlrs_dev = (pios_openlrs_t) olrs_id;

	PIOS_Assert(pios_openlrs_validate(openlrs_dev));

	openlrs_dev->tx_out_cb = tx_out_cb;
	openlrs_dev->tx_out_context = context;
}

static bool olrs_available(uintptr_t olrs_id)
{
	pios_openlrs_t openlrs_dev = (pios_openlrs_t) olrs_id;

	PIOS_Assert(pios_openlrs_validate(openlrs_dev));

	if (openlrs_dev->linkLossTimeMs) {
		return false;
	}

	if (!openlrs_dev->link_acquired) {
		return false;
	}

	return true;
}

const struct pios_com_driver pios_openlrs_com_driver = {
	.bind_tx_cb = olrs_bind_tx,
	.bind_rx_cb = olrs_bind_rx,
	.available  = olrs_available,
};

const uint32_t packet_timeout_us = 1000;
const uint32_t packet_rssi_time_us = 1500;

static uint32_t min_freq(HwSharedRfBandOptions band)
{
	switch (band) {
	default:
	case HWSHARED_RFBAND_433:
		return MIN_RFM_FREQUENCY_433;
	case HWSHARED_RFBAND_868:
		return MIN_RFM_FREQUENCY_868;
	case HWSHARED_RFBAND_915:
		return MIN_RFM_FREQUENCY_915;
	}
}

static uint32_t max_freq(HwSharedRfBandOptions band)
{
	switch (band) {
	default:
	case HWSHARED_RFBAND_433:
		return MAX_RFM_FREQUENCY_433;
	case HWSHARED_RFBAND_868:
		return MAX_RFM_FREQUENCY_868;
	case HWSHARED_RFBAND_915:
		return MAX_RFM_FREQUENCY_915;
	}
}

static uint32_t def_carrier_freq(HwSharedRfBandOptions band)
{
	switch (band) {
	default:
	case HWSHARED_RFBAND_433:
		return DEFAULT_CARRIER_FREQUENCY_433;
	case HWSHARED_RFBAND_868:
		return DEFAULT_CARRIER_FREQUENCY_868;
	case HWSHARED_RFBAND_915:
		return DEFAULT_CARRIER_FREQUENCY_915;
	}
}

static uint32_t binding_freq(HwSharedRfBandOptions band)
{
	switch (band) {
	default:
	case HWSHARED_RFBAND_433:
		return BINDING_FREQUENCY_433;
	case HWSHARED_RFBAND_868:
		return BINDING_FREQUENCY_868;
	case HWSHARED_RFBAND_915:
		return BINDING_FREQUENCY_915;
	}
}

/*****************************************************************************
* OpenLRS data formatting utilities
*****************************************************************************/

static uint8_t get_control_packet_size(struct bind_data *bd)
{
	return openlrs_pktsizes[(bd->flags & 0x07)];
}

// Sending a x byte packet on bps y takes about (emperical)
// usec = (x + 15) * 8200000 / baudrate
//
// There's 5 bytes of preamble, 4 bytes of header, 1 byte of length,
// 2 bytes of sync word, 2 bytes of CRC, yielding 14 bytes.
// 15 is used here, for a small margin.
//
// Similarly, 8 bits/byte, 1000000 microseconds/second + 2.5% for
// margin.  The values chosen here have interoperability
// considerations, so leave them alone :D
#define BYTES_AT_BAUD_TO_USEC(bytes, bps, div) ((uint32_t)((bytes) + (div ? 20 : 15)) * 8 * 1025000L / (uint32_t)(bps))

static uint32_t get_control_packet_time(struct bind_data *bd)
{
	return (BYTES_AT_BAUD_TO_USEC(get_control_packet_size(bd),
				modem_params[bd->modem_params].bps,
				bd->flags & DIVERSITY_ENABLED) + 2000);
}

static uint32_t get_nominal_packet_interval(struct bind_data *bd)
{
	uint32_t ret = get_control_packet_time(bd);

	if (bd->flags & TELEMETRY_MASK) {
		ret += (BYTES_AT_BAUD_TO_USEC(TELEMETRY_PACKETSIZE, modem_params[bd->modem_params].bps, bd->flags&DIVERSITY_ENABLED) + 1000);
	}

	// round up to ms
	ret = ((ret + 999) / 1000) * 1000;

	return ret;
}

static int pack_channels(uint8_t config, int16_t *ppm, uint8_t *buf)
{
	config &= 7;

	uint8_t *p = buf;

	/* Pack 4 channels into 5 bytes */
	for (int i = 0; i <= (config/2); i++) {
		*(p++) = ppm[0];
		*(p++) = ppm[1];
		*(p++) = ppm[2];
		*(p++) = ppm[3];
		*(p++) =
			((ppm[0] & 0x300) >> 8) |
			((ppm[1] & 0x300) >> 6) |
			((ppm[2] & 0x300) >> 4) |
			((ppm[3] & 0x300) >> 2);

		ppm += 4;
	}

	/* "Switch" channel */
	if (config & 1) {
		/* XXX TODO implement switch */
		*(p++) = 0;
	}

	return p - buf;
}

//! Apply the OpenLRS rescaling to the channels
static void txscale_channels(int16_t *chan, int16_t nominal_min, int16_t nominal_max)
{
	if (nominal_min == nominal_max) {
		/* Prevent divide by zero */
		nominal_max++;
	}

	for (uint32_t i = 0; i < OPENLRS_PPM_NUM_CHANNELS; i++) {
		int16_t x = chan[i];

		/*
		 * We have 1024 counts.  The middle 1000 should correspond to
		 * 1000..1999, the "normal range".  The extra 12 are scaled by
		 * a factor of 16 (from original openlrsng) to make the
		 * overall range from -19% to 119%
		 *
		 * In other words, the "normal resolution" is 0.1%; the
		 * extended resolution is in 1.6% chunks.
		 */

		float scaled = x;
		scaled -= nominal_min;
		scaled /= (nominal_max - nominal_min);

		const float scale_edge = (12*16) / 1000.0f; /* .192 */

		if (scaled < -scale_edge) {
			x = 0;
		} else if (scaled < 0) {
			/* [-scale_edge, 0) mapped to 0..12 */
			x = ((scaled + scale_edge) / scale_edge) * 12;
		} else if (scaled < 1) {
			/* [0 .. 1) mapped to 12..1011 */
			x = scaled * 1000 + 12;
		} else if (scaled < (1 + scale_edge)) {
			/* [1, 1 + scale_edge) mapped to 1011..1023 */
			x = ((scaled - 1) / scale_edge) * 12 + 1011;
		} else {
			x = 1023;
		}

		chan[i] = x;
	}
}

DONT_BUILD_IF(OPENLRS_PPM_NUM_CHANNELS < 20, NotEnoughChannelSpaceForAllConfigs);

static void unpack_channels(uint8_t config, int16_t *ppm, uint8_t *p)
{
	/* Look at least 3 significant bit of config.
	 * LSB picks whether 4 switch channels are around.
	 * other 2 bits are a number between 1 and 4 for number of
	 * 4 channel groups.
	 *
	 * Therefore there can be up to 20 channels.
	 */

	config &= 7;
	/* Ordinary channels are 4ch packed strangely in 5 bytes */
	for (int i = 0; i<=(config/2); i++) {
		ppm[0] = ((p[4] << 8) & 0x300) | p[0];
		ppm[1] = ((p[4] << 6) & 0x300) | p[1];
		ppm[2] = ((p[4] << 4) & 0x300) | p[2];
		ppm[3] = ((p[4] << 2) & 0x300) | p[3];
		p += 5;
		ppm += 4;
	}

	if (config & 1) { // 4ch packed in 1 byte;
		ppm[0] = ((p[0] >> 6) & 3) * 333 + 12;
		ppm[1] = ((p[0] >> 4) & 3) * 333 + 12;
		ppm[2] = ((p[0] >> 2) & 3) * 333 + 12;
		ppm[3] = ((p[0] >> 0) & 3) * 333 + 12;
	}
}

//! Apply the OpenLRS rescaling to the channels
static void rxscale_channels(int16_t control[])
{
	for (uint32_t i = 0; i < OPENLRS_PPM_NUM_CHANNELS; i++) {
		int16_t x = control[i];
		int16_t ret;

		if (x < 12) {
			ret = 808 + x * 16;
		} else if (x < 1012) {
			ret = x + 988;
		} else if (x < 1024) {
			ret = 2000 + (x - 1011) * 16;
		} else {
			ret = 2192;
		}

		control[i] = ret;
	}
}

static bool pios_openlrs_form_telemetry(pios_openlrs_t openlrs_dev,
		uint8_t *tx_buf, uint8_t rssi, bool only_serial)
{
	tx_buf[0] &= 0xc0; // Preserve top two sequence bits

	// Check for data on serial link
	uint8_t bytes = 0;
	// Append data from the com interface if applicable.
	if (openlrs_dev->tx_out_cb) {
		// Try to get some data to send
		bool need_yield = false;
		bytes = (openlrs_dev->tx_out_cb)(openlrs_dev->tx_out_context, &tx_buf[1], 8, NULL, &need_yield);
	}

	if (bytes > 0) {
		tx_buf[0] |= (0x37 + bytes);

		for (int i = bytes+1; i < TELEMETRY_PACKETSIZE; i++) {
			tx_buf[i] = 0;
		}
	} else if (only_serial) {
		return false;
	} else {
		// tx_buf[0] lowest 6 bits left at 0
		tx_buf[1] = rssi;
		if (FlightBatteryStateHandle()) {
			FlightBatteryStateData bat;
			FlightBatteryStateGet(&bat);
			// FrSky protocol normally uses 3.3V at
			// 255 but divider from display can be
			// set internally
			if (bat.DetectedCellCount) {
				/* Use detected cell count to
				 * maximize useful resolution
				 */
				tx_buf[2] = (uint8_t) (bat.Voltage / bat.DetectedCellCount / 5.0f * 255);
			} else {
				tx_buf[2] = (uint8_t) (bat.Voltage / 25.0f * 255);
			}
			tx_buf[3] = (uint8_t) (bat.Current / 60.0f * 255);
		} else {
			tx_buf[2] = 0;
			tx_buf[3] = 0;
		}

		/*
		 * 4-5: This used to be AFCC information, which
		 * was never used.  Deprecated.
		 */
		tx_buf[4] = 0;
		tx_buf[5] = 0;
		tx_buf[6] = count_set_bits(openlrs_status.LinkQuality & 0x7fff);
		tx_buf[7] = 0;
		tx_buf[8] = 0;
	}

	return true;
}

static uint32_t millis()
{
	return PIOS_Thread_Systime();
}

/*****************************************************************************
* OpenLRS hardware access
*****************************************************************************/

static uint8_t beaconGetRSSI(pios_openlrs_t openlrs_dev)
{
	uint16_t rssiSUM = 0;

	rfm22_set_frequency(openlrs_dev, openlrs_dev->beacon_frequency);

	PIOS_Thread_Sleep(1);
	rssiSUM += rfm22_get_rssi(openlrs_dev);
	PIOS_Thread_Sleep(1);
	rssiSUM += rfm22_get_rssi(openlrs_dev);
	PIOS_Thread_Sleep(1);
	rssiSUM += rfm22_get_rssi(openlrs_dev);
	PIOS_Thread_Sleep(1);
	rssiSUM += rfm22_get_rssi(openlrs_dev);

	rfm22_set_frequency(openlrs_dev, openlrs_dev->bind_data.rf_frequency);

	return rssiSUM / 4;
}

/*****************************************************************************
 * High level OpenLRS functions
 *****************************************************************************/

static bool pios_openlrs_bind_transmit_step(pios_openlrs_t openlrs_dev)
{
	if (openlrs_dev->bind_data.version != BINDING_VERSION) {
		/* decline to send bind with invalid configuration */
		return false;
	}

	uint32_t start = millis();

	/* Select bind channel, bind power, and bind packet header */
	rfm22_init(openlrs_dev, true);
	PIOS_Thread_Sleep(5);

	rfm22_tx_packet(openlrs_dev,
			&openlrs_dev->bind_data,
			sizeof(openlrs_dev->bind_data));

	rfm22_rx_reset(openlrs_dev, false);

	DEBUG_PRINTF(2,"Waiting bind ack\r\n");

	while ((millis() - start) < 100) {
#if defined(PIOS_INCLUDE_WDG) && defined(PIOS_WDG_RFM22B)
		// Update the watchdog timer
		PIOS_WDG_UpdateFlag(PIOS_WDG_RFM22B);
#endif /* PIOS_WDG_RFM22B */

		PIOS_Thread_Sleep(1);

		if (openlrs_dev->rf_mode == Received) {
			DEBUG_PRINTF(2,"Got pkt\r\n");

			uint8_t recv_byte;

			if (rfm22_rx_packet(openlrs_dev, &recv_byte, 1) &&
					(recv_byte == 'B')) {
				DEBUG_PRINTF(2, "Received bind ack\r\n");

				return true;
			}

			rfm22_rx_reset(openlrs_dev, false);
		}
	}

	return false;
}

static void pios_openlrs_config_to_bind_data(pios_openlrs_t openlrs_dev)
{
	OpenLRSData binding;
	OpenLRSGet(&binding);

	if (binding.version == BINDING_VERSION) {
		openlrs_dev->bind_data.hdr = 'b';
		openlrs_dev->bind_data.version = binding.version;
		openlrs_dev->bind_data.reserved = 0;
		openlrs_dev->bind_data.rf_frequency = binding.rf_frequency;
		openlrs_dev->bind_data.rf_magic = binding.rf_magic;
		openlrs_dev->bind_data.rf_power = binding.rf_power;
		openlrs_dev->bind_data.rf_channel_spacing = binding.rf_channel_spacing;
		openlrs_dev->bind_data.modem_params = binding.modem_params;
		openlrs_dev->bind_data.flags = binding.flags;
		for (uint32_t i = 0; i < OPENLRS_HOPCHANNEL_NUMELEM; i++)
			openlrs_dev->bind_data.hopchannel[i] = binding.hopchannel[i];
	}

	// Also copy beacon settings over to device config.
	openlrs_dev->beacon_frequency = binding.beacon_frequency;
	openlrs_dev->beacon_delay = binding.beacon_delay;
	openlrs_dev->beacon_period = binding.beacon_period;

	openlrs_dev->failsafeDelay = binding.failsafe_delay;

	openlrs_dev->scale_min = binding.tx_scale_min;
	openlrs_dev->scale_max = binding.tx_scale_max;

	openlrs_dev->tx_source = binding.tx_source;
}

static bool pios_openlrs_bind_data_to_config(pios_openlrs_t openlrs_dev)
{
	if (openlrs_dev->bind_data.hdr == 'b') {
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
		if (2 <= DEBUG_LEVEL && pios_com_debug_id > 0) {
			DEBUG_PRINTF(2, "Binding settings:\r\n");
			PIOS_Thread_Sleep(10);
			DEBUG_PRINTF(2, "  version: %d\r\n", openlrs_dev->bind_data.version);
			PIOS_Thread_Sleep(10);
			DEBUG_PRINTF(2, "  rf_frequency: %d\r\n", openlrs_dev->bind_data.rf_frequency);
			PIOS_Thread_Sleep(10);
			DEBUG_PRINTF(2, "  rf_power: %d\r\n", openlrs_dev->bind_data.rf_power);
			PIOS_Thread_Sleep(10);
			DEBUG_PRINTF(2, "  rf_channel_spacing: %d\r\n", openlrs_dev->bind_data.rf_channel_spacing);
			PIOS_Thread_Sleep(10);
			DEBUG_PRINTF(2, "  modem_params: %d\r\n", openlrs_dev->bind_data.modem_params);
			PIOS_Thread_Sleep(10);
			DEBUG_PRINTF(2, "  flags: %d\r\n", openlrs_dev->bind_data.flags);
			PIOS_Thread_Sleep(10);

			for (uint32_t i = 0; i < MAXHOPS; i++) {
				DEBUG_PRINTF(2, "    hop channel: %d\r\n", openlrs_dev->bind_data.hopchannel[i]);
				PIOS_Thread_Sleep(10);
			}
		}
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */

		if (openlrs_dev->bind_data.version == BINDING_VERSION) {
			OpenLRSData binding;
			OpenLRSGet(&binding);
			binding.version = openlrs_dev->bind_data.version;
			binding.rf_frequency = openlrs_dev->bind_data.rf_frequency;
			binding.rf_magic = openlrs_dev->bind_data.rf_magic;
			binding.rf_power = openlrs_dev->bind_data.rf_power;
			binding.rf_channel_spacing = openlrs_dev->bind_data.rf_channel_spacing;
			binding.modem_params = openlrs_dev->bind_data.modem_params;
			binding.flags = openlrs_dev->bind_data.flags;
			for (uint32_t i = 0; i < OPENLRS_HOPCHANNEL_NUMELEM; i++)
				binding.hopchannel[i] = openlrs_dev->bind_data.hopchannel[i];
			binding.beacon_frequency = openlrs_dev->beacon_frequency;
			binding.beacon_delay = openlrs_dev->beacon_delay;
			binding.beacon_period = openlrs_dev->beacon_period;
			OpenLRSSet(&binding);
			UAVObjSave(OpenLRSHandle(), 0);

			DEBUG_PRINTF(2, "Saved bind data\r\n");
#if defined(PIOS_LED_LINK)
			PIOS_ANNUNC_Toggle(PIOS_LED_LINK);
#endif /* PIOS_LED_LINK */

			return true;
		}
	}

	return false;
}

static bool pios_openlrs_bind_receive(pios_openlrs_t openlrs_dev,
		uint32_t timeout)
{
	uint32_t start = millis();
	uint8_t txb;
	rfm22_init(openlrs_dev, true);
	rfm22_rx_reset(openlrs_dev, true);
	DEBUG_PRINTF(2,"Waiting bind\r\n");

	uint32_t i = 0;

	while ((!timeout) || ((millis() - start) < timeout)) {
		PIOS_Thread_Sleep(1);
#if defined(PIOS_INCLUDE_WDG) && defined(PIOS_WDG_RFM22B)
		// Update the watchdog timer
		PIOS_WDG_UpdateFlag(PIOS_WDG_RFM22B);
#endif /* PIOS_WDG_RFM22B */

		if (i++ % 100 == 0) {
			DEBUG_PRINTF(2,"Waiting bind\r\n");

#if defined(PIOS_LED_LINK)
			PIOS_ANNUNC_Toggle(PIOS_LED_LINK);
#endif /* PIOS_LED_LINK */
		}
		if (openlrs_dev->rf_mode == Received) {
			DEBUG_PRINTF(2,"Got pkt\r\n");

			bool ok = rfm22_rx_packet(openlrs_dev,
					&openlrs_dev->bind_data,
					sizeof(openlrs_dev->bind_data));

			if (ok && pios_openlrs_bind_data_to_config(openlrs_dev)) {
				DEBUG_PRINTF(2, "data good\r\n");

				/* Acknowledge binding */
				txb = 'B';
				rfm22_tx_packet(openlrs_dev, &txb, 1);

				return true;
			}

			rfm22_rx_reset(openlrs_dev, false);
		}
	}

	return false;
}

#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
static void printVersion(uint16_t v)
{
	char ver[8];
	ver[0] = '0' + ((v >> 8) & 0x0f);
	ver[1] = '.';
	ver[2] = '0' + ((v >> 4) & 0x0f);
	if (v & 0x0f) {
		ver[3] = '.';
		ver[4] = '0' + (v & 0x0f);
		ver[5] = '\r';
		ver[6] = '\n';
		ver[7] = '\0';
	} else {
		ver[3] = '\r';
		ver[4] = '\n';
		ver[5] = '\0';
	}
	DEBUG_PRINTF(2, ver);
}
#endif

static void pios_openlrs_setup(pios_openlrs_t openlrs_dev)
{
	DEBUG_PRINTF(2,"OpenLRSng RX setup starting.\r\n");
	PIOS_Thread_Sleep(5);
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
	printVersion(OPENLRSNG_VERSION);
#endif

	DEBUG_PRINTF(2,"Entering normal mode\r\n");

	// Count hopchannels as we need it later
	openlrs_dev->hopcount = 0;
	while ((openlrs_dev->hopcount < MAXHOPS) &&
			(openlrs_dev->bind_data.hopchannel[openlrs_dev->hopcount] != 0)) {
		openlrs_dev->hopcount++;
	}

	// Configure for normal operation.
	rfm22_init(openlrs_dev, false);

	pios_openlrs_do_hop(openlrs_dev);

	openlrs_dev->link_acquired = false;
	openlrs_dev->lastPacketTimeUs = PIOS_DELAY_GetuS();

	DEBUG_PRINTF(2,"OpenLRSng RX setup complete\r\n");
}

static void pios_openlrs_rx_update_rssi(pios_openlrs_t openlrs_dev,
		uint8_t rssi, bool have_frame)
{
	openlrs_status.LinkQuality <<= 1;
	openlrs_status.LinkQuality |= !!have_frame;

	if (have_frame) {
		openlrs_status.LastRSSI = rssi;
	} else {
		/*
		 * If we weren't able to sample RSSI, don't just carry the
		 * old value forward indefinitely.
		 */
		openlrs_status.LastRSSI >>= 1;
	}
}

static bool pios_openlrs_rx_frame(pios_openlrs_t openlrs_dev, uint8_t rssi)
{
	uint8_t *tx_buf = openlrs_dev->tx_buf;
	uint8_t rx_buf[MAX_CONTROL_PACKET_SIZE];

	int ctrl_size = get_control_packet_size(&openlrs_dev->bind_data);

	// Read the packet from RFM22b
	int size = rfm22_rx_packet_variable(openlrs_dev, rx_buf, ctrl_size);

	if (size <= 0) {
		return false;
	}

	pios_openlrs_rx_update_rssi(openlrs_dev, rssi, true);

#if defined(PIOS_LED_LINK)
	PIOS_ANNUNC_Toggle(PIOS_LED_LINK);
#endif /* PIOS_LED_LINK */

	if ((rx_buf[0] & 0x3e) == 0x00) {
		// This flag indicates receiving control data

		if (size != ctrl_size) {
			// These size mismatches should never happen because
			// we are protected by a CRC, and shared magic hdr.
			return false;
		}

		unpack_channels(openlrs_dev->bind_data.flags,
				openlrs_dev->ppm, rx_buf + 1);
		rxscale_channels(openlrs_dev->ppm);

		// Call the control received callback if it's available.
		if (openlrs_dev->openlrs_rcvr_id) {
#if defined(PIOS_INCLUDE_OPENLRS_RCVR)
			PIOS_OpenLRS_Rcvr_UpdateChannels(
					openlrs_dev->openlrs_rcvr_id,
					openlrs_dev->ppm);
#endif
		}
	} else {
		if (size != TELEMETRY_PACKETSIZE) {
			return false;
		}

		// Not control data. Push into serial RX buffer.
		if (((rx_buf[0] & 0x38) == 0x38) &&
				((rx_buf[0] ^ tx_buf[0]) & 0x80)) {
			// We got new data... (not retransmission)
			bool rx_need_yield;
			uint8_t data_len = (rx_buf[0] & 7) + 1;
			if (openlrs_dev->rx_in_cb) {
				(openlrs_dev->rx_in_cb)(openlrs_dev->rx_in_context,
						&rx_buf[1], data_len,
						NULL, &rx_need_yield);
			}
		}

		/* Set acknowledge bit to match */
		if (rx_buf[0] & 0x80) {
			openlrs_dev->tx_buf[0] |= 0x80;
		} else {
			openlrs_dev->tx_buf[0] &= ~0x80;
		}
	}

	// When telemetry is enabled we ack packets and send info about FC back
	if (openlrs_dev->bind_data.flags & TELEMETRY_MASK) {
		if ((tx_buf[0] ^ rx_buf[0]) & 0x40) {
			// Last message was lost... resend, unless it
			// is just ordinary rx telemetry in which
			// case we can update it.

			if ((tx_buf[0] & 0x3F) == 0) {
				// Don't swap telem sequence bit though.
				pios_openlrs_form_telemetry(openlrs_dev,
						tx_buf, rssi, false);
			}
		} else {
			tx_buf[0] ^= 0x40; // Swap telem sequence
			pios_openlrs_form_telemetry(openlrs_dev,
					tx_buf, rssi, false);
		}

		// XXX consider proving it's safe to send shorter frames
		// with existing impl, will improve margins on timing
		// This will block until sent
		rfm22_tx_packet(openlrs_dev, tx_buf, TELEMETRY_PACKETSIZE);
	}

	// Once a packet has been processed, flip back into receiving mode
	rfm22_rx_reset(openlrs_dev, false);

	return true;
}

static void pios_openlrs_do_hop(pios_openlrs_t openlrs_dev)
{
	openlrs_dev->rf_channel++;

	if (openlrs_dev->rf_channel >= openlrs_dev->hopcount) {
		openlrs_dev->rf_channel = 0;
	}

	if (openlrs_dev->beacon_armed) {
		// Listen for RSSI on beacon channel briefly for
		// a sharp rising edge in RSSI to 'trigger' us
		uint8_t rssi = beaconGetRSSI(openlrs_dev) << 2;

		if ((openlrs_dev->beacon_rssi_avg + 80) < rssi) {
			openlrs_dev->nextBeaconTimeMs =
				millis() + 1000L;
		}

		openlrs_dev->beacon_rssi_avg =
			(openlrs_dev->beacon_rssi_avg * 3 + rssi) >> 2;
	}

	rfm22_set_channel(openlrs_dev, openlrs_dev->rf_channel);
}

static void pios_openlrs_rx_after_receive(pios_openlrs_t openlrs_dev,
		bool have_frame)
{
	uint32_t timeUs, timeMs;

	/* We hop channels here, unless we're hopping slowly and
	 * it's not time yet.
	 */
	bool willhop = true;

#if defined(PIOS_INCLUDE_WDG) && defined(PIOS_WDG_RFM22B)
	// Update the watchdog timer
	PIOS_WDG_UpdateFlag(PIOS_WDG_RFM22B);
#endif /* PIOS_WDG_RFM22B */

	timeUs = PIOS_DELAY_GetuS();
	timeMs = millis();

	DEBUG_PRINTF(2,"Time: %d\r\n", timeUs);

	// For missing packets to properly trigger a well timed
	// channel hop, this method should be called at the
	// appropriate time after the packet was expected to trigger
	// this path.
	if (have_frame) {
		// Flag to indicate we ever got a frame
		openlrs_dev->link_acquired = true;

		openlrs_dev->beacon_armed = false;
		openlrs_status.FailsafeActive = OPENLRSSTATUS_FAILSAFEACTIVE_INACTIVE;
		openlrs_dev->numberOfLostPackets = 0;
		openlrs_dev->nextBeaconTimeMs = 0;

		openlrs_dev->lastPacketTimeUs = timeUs;
		openlrs_dev->linkLossTimeMs = timeMs;
	} else if (openlrs_dev->numberOfLostPackets < openlrs_dev->hopcount) {
		DEBUG_PRINTF(2,"OLRS WARN: Lost packet: %d\r\n", openlrs_dev->numberOfLostPackets);
		/* We lost packet, execute normally timed hop */
		openlrs_dev->numberOfLostPackets++;
		openlrs_dev->lastPacketTimeUs +=
			get_nominal_packet_interval(&openlrs_dev->bind_data);
	} else if ((openlrs_dev->numberOfLostPackets >= openlrs_dev->hopcount) &&
			(PIOS_DELAY_GetuSSince(openlrs_dev->lastPacketTimeUs) >
			(get_nominal_packet_interval(&openlrs_dev->bind_data)
					* (openlrs_dev->hopcount + 1)))) {
		DEBUG_PRINTF(2,"OLRS WARN: Trying to sync\r\n");

		// hop slowly to allow resync with TX
		openlrs_dev->lastPacketTimeUs = timeUs;
	} else {
		/* Don't have link, but it's not time for slow hop yet */
		willhop = false;
	}

	if (openlrs_dev->link_acquired && openlrs_dev->numberOfLostPackets) {
#if defined(PIOS_LED_LINK)
		PIOS_ANNUNC_Off(PIOS_LED_LINK);
#endif /* PIOS_LED_LINK */

		if (openlrs_dev->failsafeDelay &&
				(openlrs_status.FailsafeActive == OPENLRSSTATUS_FAILSAFEACTIVE_INACTIVE) &&
				((timeMs - openlrs_dev->linkLossTimeMs) > ((uint32_t) openlrs_dev->failsafeDelay))) {
			DEBUG_PRINTF(2,"Failsafe activated: %d %d\r\n", timeMs, openlrs_dev->linkLossTimeMs);
			openlrs_status.FailsafeActive = OPENLRSSTATUS_FAILSAFEACTIVE_ACTIVE;

			openlrs_dev->nextBeaconTimeMs = (timeMs + 1000UL * openlrs_dev->beacon_delay) | 1; //beacon activating...
		}

		if ((openlrs_dev->beacon_frequency) && (openlrs_dev->nextBeaconTimeMs) &&
				((timeMs - openlrs_dev->nextBeaconTimeMs) < 0x80000000)) {

			// Indicate that the beacon is now active so we can
			// trigger extra ones by RF.
			openlrs_dev->beacon_armed = true;

			DEBUG_PRINTF(2,"Beacon time: %d\r\n", openlrs_dev->nextBeaconTimeMs);
			// Only beacon when vehicle is disarmed
			// (This should be reconsidered, because it means we
			// can't beacon when always armed, etc.)
			uint8_t armed;
			FlightStatusArmedGet(&armed);
			if (armed == FLIGHTSTATUS_ARMED_DISARMED) {
				rfm22_beacon_send(openlrs_dev, false);
				rfm22_init(openlrs_dev, false);
				rfm22_rx_reset(openlrs_dev, false);
				openlrs_dev->nextBeaconTimeMs = (timeMs + 1000UL * openlrs_dev->beacon_period) | 1; // avoid 0 in time
			}
		}
	}

	if (willhop) {
		pios_openlrs_do_hop(openlrs_dev);
		rfm22_rx_reset(openlrs_dev, false);
	}

	// Update UAVO
	OpenLRSStatusSet(&openlrs_status);
}

uint8_t PIOS_OpenLRS_RSSI_Get(void)
{
	if (openlrs_status.FailsafeActive == OPENLRSSTATUS_FAILSAFEACTIVE_ACTIVE) {
		return 0;
	} else {
		// Check object handle exists
		if (OpenLRSHandle() == NULL)
			return 0;

		uint8_t rssi_type;
		OpenLRSRSSI_TypeGet(&rssi_type);

		uint16_t LQ = count_set_bits(openlrs_status.LinkQuality & 0x7fff);

		switch (rssi_type) {
		case OPENLRS_RSSI_TYPE_COMBINED:
			if (LQ == 15) {
				return (openlrs_status.LastRSSI >> 1)+128;
			} else if (LQ == 0) {
				return 0;
			} else {
				return LQ * 9 +
					(openlrs_status.LastRSSI >> 5);
			}
		case OPENLRS_RSSI_TYPE_RSSI:
			return openlrs_status.LastRSSI;
		case OPENLRS_RSSI_TYPE_LINKQUALITY:
			return LQ << 4;
		default:
			return 0;
		}
	}
}

/*****************************************************************************
 * control Code
 *****************************************************************************/

/**
 * Register a OpenLRS_Rcvr interface to inform of control packets
 *
 * @param[in] rfm22b_dev     The RFM22B device ID.
 * @param[in] rfm22b_rcvr_id The receiver device to inform of control packets
 */
void PIOS_OpenLRS_RegisterRcvr(pios_openlrs_t openlrs_dev,
		uintptr_t openlrs_rcvr_id)
{
	PIOS_Assert(pios_openlrs_validate(openlrs_dev));

	openlrs_dev->openlrs_rcvr_id = openlrs_rcvr_id;
}

/*****************************************************************************
* Task and device setup
*****************************************************************************/

/**
 * Initialise an RFM22B device
 *
 * @param[out] rfm22b_id  A pointer to store the device ID in.
 * @param[in] spi_id  The SPI bus index.
 * @param[in] slave_num  The SPI bus slave number.
 * @param[in] cfg  The device configuration.
 */
int32_t PIOS_OpenLRS_Init(pios_openlrs_t *openlrs_id,
		pios_spi_t spi_id, uint32_t slave_num,
		const struct pios_openlrs_cfg *cfg,
		HwSharedRfBandOptions rf_band)
{
	PIOS_DEBUG_Assert(rfm22b_id);
	PIOS_DEBUG_Assert(cfg);

	// Allocate the device structure.
	pios_openlrs_t openlrs_dev = pios_openlrs_alloc();
	if (!openlrs_dev) {
		return -1;
	}

	// Store the SPI handle
	openlrs_dev->slave_num = slave_num;
	openlrs_dev->spi_id = spi_id;

	// and the frequency
	openlrs_dev->band = rf_band;

	// Before initializing everything, make sure device found
	uint8_t device_type = rfm22_read(openlrs_dev, RFM22_DEVICE_TYPE) & RFM22_DT_MASK;
	if (device_type != 0x08)
		return -1;

	OpenLRSInitialize();
	OpenLRSStatusInitialize();

	// Bind the configuration to the device instance
	openlrs_dev->cfg = *cfg;

	// Convert UAVO configuration to device runtime config
	// XXX register for updates!  change at runtime.
	pios_openlrs_config_to_bind_data(openlrs_dev);

	*openlrs_id = openlrs_dev;

	return 0;
}

int32_t PIOS_OpenLRS_Start(pios_openlrs_t openlrs_dev)
{
	// Initialize the external interrupt.
	PIOS_EXTI_Init(openlrs_dev->cfg.exti_cfg);

	// Start the receiver task.
	OpenLRSroleOptions role;

	OpenLRSroleGet(&role);

	switch (role) {
		case OPENLRS_ROLE_RX:
			openlrs_dev->taskHandle =
				PIOS_Thread_Create(pios_openlrs_rx_task,
					"PIOS_OpenLRS_Task", STACK_SIZE_BYTES,
					(void *)openlrs_dev, TASK_PRIORITY);
			TaskMonitorAdd(TASKINFO_RUNNING_MODEM,
					openlrs_dev->taskHandle);
			break;
		case OPENLRS_ROLE_TX:
			openlrs_dev->taskHandle =
				PIOS_Thread_Create(pios_openlrs_tx_task,
					"PIOS_OpenLRS_Task", STACK_SIZE_BYTES,
					(void *)openlrs_dev, TASK_PRIORITY);
			TaskMonitorAdd(TASKINFO_RUNNING_MODEM,
					openlrs_dev->taskHandle);
			break;
		case OPENLRS_ROLE_DISABLED:
			/* handle putting radio hw into safe state. */
			rfm22_disable(openlrs_dev);
			break;
	}

	return 0;
}

static int pios_openlrs_form_control_frame(pios_openlrs_t openlrs_dev)
{
	int16_t channels[OPENLRS_PPM_NUM_CHANNELS];

	for (int i=0; i < OPENLRS_PPM_NUM_CHANNELS; i++) {
		uintptr_t rcvr =
			PIOS_HAL_GetReceiver(openlrs_dev->tx_source);
		channels[i] = PIOS_RCVR_Read(rcvr, i+1);
	}

	txscale_channels(channels, openlrs_dev->scale_min, openlrs_dev->scale_max);
	uint8_t *tx_buf = openlrs_dev->tx_buf;

	int len = pack_channels(openlrs_dev->bind_data.flags,
		channels, tx_buf+1);

	/* Packed length + 1 byte */
	return len + 1;
}

static bool pios_openlrs_tx_receive_telemetry(pios_openlrs_t openlrs_dev)
{
	uint8_t rx_buf[TELEMETRY_PACKETSIZE];

	// Read the packet from RFM22b
	if (!rfm22_rx_packet(openlrs_dev, rx_buf,
				TELEMETRY_PACKETSIZE)) {
		return false;
	}

#if defined(PIOS_LED_LINK)
	PIOS_ANNUNC_Toggle(PIOS_LED_LINK);
#endif /* PIOS_LED_LINK */

	if ((rx_buf[0] & 0x3f) == 0) {
		/* Generic link telemetry.  Grab the data, shove it
		 * into status.
		 */

		openlrs_status.LastRSSI = rx_buf[1];
		openlrs_status.LinkQuality = (1 << (rx_buf[6])) - 1; /* fudge */

		OpenLRSStatusSet(&openlrs_status);
	} else if (((rx_buf[0] & 0x38) == 0x38) &&
			((rx_buf[0] ^ openlrs_dev->tx_buf[0]) & 0x40)) {
		bool rx_need_yield;
		uint8_t data_len = (rx_buf[0] & 7) + 1;
		if (openlrs_dev->rx_in_cb) {
			(openlrs_dev->rx_in_cb)(openlrs_dev->rx_in_context,
					&rx_buf[1], data_len,
					NULL, &rx_need_yield);
		}
	}

	/* Set acknowledge bit to match */
	if (rx_buf[0] & 0x40) {
		openlrs_dev->tx_buf[0] |= 0x40;
	} else {
		openlrs_dev->tx_buf[0] &= ~0x40;
	}

	return true;

}

static void pios_openlrs_tx_frame(pios_openlrs_t openlrs_dev)
{
	rfm22_rx_reset(openlrs_dev, false);

	uint32_t interval =
		get_nominal_packet_interval(&openlrs_dev->bind_data) + 500;

	pios_openlrs_do_hop(openlrs_dev);

	bool telem_uplink = false;

	int len = TELEMETRY_PACKETSIZE;

	openlrs_dev->tx_buf[0] &= 0xc0; // Preserve top two sequence bits

	if (openlrs_dev->tx_ok_to_telemeter) {
		openlrs_dev->tx_ok_to_telemeter = false;

		/* XXX resends, etc... tricky */

		telem_uplink = pios_openlrs_form_telemetry(openlrs_dev,
					openlrs_dev->tx_buf, 0, true);

		if (telem_uplink) {
			openlrs_dev->tx_buf[0] ^= 0x80;
		}
	}

	if (!telem_uplink) {
		len = pios_openlrs_form_control_frame(openlrs_dev);
	}

	// Now we want to delay until the next frame is due.
	while (PIOS_DELAY_GetuSSince(openlrs_dev->lastPacketTimeUs) <
			interval) {
		PIOS_Thread_Sleep(1);
	}

	openlrs_dev->lastPacketTimeUs = PIOS_DELAY_GetuS();

	rfm22_tx_packet(openlrs_dev, openlrs_dev->tx_buf, len);

	if (!(openlrs_dev->bind_data.flags & TELEMETRY_MASK)) {
		return;
	}

	rfm22_rx_reset(openlrs_dev, false);

	uint32_t wait_time = interval - PIOS_DELAY_GetuSSince(
			openlrs_dev->lastPacketTimeUs);

	if ((wait_time > 100000) || (wait_time < 500)) {
		/* Only do telemetry if time remaining is sane. */
		return;
	}

	bool have_interrupt = wait_interrupt(openlrs_dev, wait_time);

	bool got_packet = false;

	if (have_interrupt) {
		if (pios_openlrs_tx_receive_telemetry(openlrs_dev)) {
			if (!telem_uplink) {
				/* It's OK to telemeter when we receive
				 * a downlink telemetry message.. and we
				 * didn't telemeter this time around.
				 */
				openlrs_dev->tx_ok_to_telemeter = true;
			}

			got_packet = true;
		}
	}

	if (!got_packet) {
		/* TODO: link-beep functionality goes here */
		if (openlrs_dev->numberOfLostPackets < openlrs_dev->hopcount) {
			openlrs_dev->numberOfLostPackets++;
		} else {
#if defined(PIOS_LED_LINK)
			/* Ensure link light is off */
			PIOS_ANNUNC_Off(PIOS_LED_LINK);
#endif
			if (!openlrs_dev->linkLossTimeMs) {
				openlrs_dev->linkLossTimeMs =
					PIOS_Thread_Systime();
			}
		}
	} else {
		openlrs_dev->numberOfLostPackets = 0;
		openlrs_dev->linkLossTimeMs = 0;
		openlrs_dev->link_acquired = true;
	}
}

static void pios_openlrs_tx_task(void *parameters)
{
	pios_openlrs_t openlrs_dev = (pios_openlrs_t)parameters;

	// Register the watchdog timer for the radio driver task
#if defined(PIOS_INCLUDE_WDG) && defined(PIOS_WDG_RFM22B)
	PIOS_WDG_RegisterFlag(PIOS_WDG_RFM22B);
#endif /* PIOS_WDG_RFM22B */

	PIOS_Assert(pios_openlrs_validate(openlrs_dev));

	bool binding = true;	/* XXX */

	pios_openlrs_setup(openlrs_dev);

	PIOS_Thread_Sleep(200);

	pios_openlrs_setup(openlrs_dev);

	int i = 0;

	while (1) {
		//rfm22_check_hang(openlrs_dev);

		if (binding) {
			if (pios_openlrs_bind_transmit_step(openlrs_dev)) {
				binding = false;
				rfm22_init(openlrs_dev, false);
			}

			/* 50 ~100ms steps, so about 6s to leave binding */
			if (i++ > 50) {
				binding = false;
				rfm22_init(openlrs_dev, false);
			}
		} else {
#if defined(PIOS_INCLUDE_WDG) && defined(PIOS_WDG_RFM22B)
			// Update the watchdog timer
			PIOS_WDG_UpdateFlag(PIOS_WDG_RFM22B);
#endif

			i = 0;
			pios_openlrs_tx_frame(openlrs_dev);
		}
	}
}

static void pios_openlrs_rx_step(pios_openlrs_t openlrs_dev)
{
	rfm22_check_hang(openlrs_dev);

	uint32_t time_since_packet_us =
		PIOS_DELAY_GetuSSince(openlrs_dev->lastPacketTimeUs);
	uint8_t rssi = 0;

	/*
	 * Schedule for a good time to sample RSSI.  This should be
	 * after frame start and the AGC has had a good chance to
	 * lock on.
	 */
	uint32_t delay_us =
		get_control_packet_time(&openlrs_dev->bind_data) -
		packet_rssi_time_us -
		time_since_packet_us + 999;

	DEBUG_PRINTF(3, "T1: %d\r\n", delay_us);

	bool have_interrupt = wait_interrupt(openlrs_dev, delay_us);

	/*
	 * We don't really expect an interrupt here.  If we have one,
	 * it means we are badly out of sync and got a frame when we
	 * didn't expect.
	 */
	if (!have_interrupt) {
		rssi = rfm22_get_rssi(openlrs_dev);

		DEBUG_PRINTF(3,
			"Sampled RSSI: %d %d\r\n",
			rssi, delay_us);
	}

	time_since_packet_us =
		PIOS_DELAY_GetuSSince(openlrs_dev->lastPacketTimeUs);

	/*
	 * Now wait until we have received a frame, or until the
	 * packet is truly late.
	 *
	 * It's worth noting that for non-telemetry modes, we need
	 * to stay very closely in sync because the TX is transmitting
	 * most of the time.  This means that we need to carefully
	 * split that "dead" non-TX time, arriving to channels slightly
	 * early, and leaving slightly late if we have not received a
	 * frame.
	 *
	 * For telemetry modes, since "we" have a timeslot the
	 * tolerances can be much more lax-- we can afford to listen
	 * into what we believe to be our telemetry timeslot and thus
	 * cope better if the TX is slower than we expect.
	 */
	delay_us = get_nominal_packet_interval(&openlrs_dev->bind_data) +
		packet_timeout_us - time_since_packet_us;
	DEBUG_PRINTF(3, "T2: %d %d\r\n", delay_us,
			time_since_packet_us);

	bool have_frame = false;

	if (have_interrupt ||
			wait_interrupt(openlrs_dev, delay_us)) {
		DEBUG_PRINTF(3, "ISR %d %d %d\r\n", delay_us,
			get_nominal_packet_interval(&openlrs_dev->bind_data),
			time_since_packet_us);

		// Process incoming data
		have_frame = pios_openlrs_rx_frame(openlrs_dev, rssi);
	} else {
		// TODO: We could delay a little bit more if we looked
		// at the hardware and see it's midway through
		// receiving a valid frame.

		// We timed out because packet was missed
		DEBUG_PRINTF(3,
			"ISR Timeout. Missed packet: %d %d %d\r\n",
			delay_us,
			get_nominal_packet_interval(&openlrs_dev->bind_data),
			time_since_packet_us);
	}

	if (!have_frame) {
		pios_openlrs_rx_update_rssi(openlrs_dev, rssi,
			false);
	}

	// Select next channel, update timers, etc.
	pios_openlrs_rx_after_receive(openlrs_dev, have_frame);

	DEBUG_PRINTF(3, "Processing time %d\r\n", PIOS_DELAY_GetuSSince(openlrs_dev->lastPacketTimeUs));
}

/**
 * The task that controls the radio state machine.
 *
 * @param[in] paramters  The task parameters.
 */
static void pios_openlrs_rx_task(void *parameters)
{
	pios_openlrs_t openlrs_dev = (pios_openlrs_t)parameters;

	// Register the watchdog timer for the radio driver task
#if defined(PIOS_INCLUDE_WDG) && defined(PIOS_WDG_RFM22B)
	PIOS_WDG_RegisterFlag(PIOS_WDG_RFM22B);
#endif /* PIOS_WDG_RFM22B */

	PIOS_Assert(pios_openlrs_validate(openlrs_dev));

	pios_openlrs_setup(openlrs_dev);

	while (openlrs_dev->bind_data.version != BINDING_VERSION) {
		pios_openlrs_bind_receive(openlrs_dev, 0);
	}

	DEBUG_PRINTF(2, "Setup complete\r\n");

	while (1) {
#if defined(PIOS_INCLUDE_WDG) && defined(PIOS_WDG_RFM22B)
		// Update the watchdog timer
		PIOS_WDG_UpdateFlag(PIOS_WDG_RFM22B);
#endif /* PIOS_WDG_RFM22B */

		pios_openlrs_rx_step(openlrs_dev);
	}
}

bool PIOS_OpenLRS_EXT_Int(pios_openlrs_t openlrs_dev)
{
	PIOS_Assert(pios_openlrs_validate(openlrs_dev));

	if (openlrs_dev->rf_mode == Transmit) {
		openlrs_dev->rf_mode = Transmitted;
	} else if (openlrs_dev->rf_mode == Receive) {
		openlrs_dev->rf_mode = Received;
	}

	// Indicate to main task that an ISR occurred
	bool woken = false;
	PIOS_Semaphore_Give_FromISR(openlrs_dev->sema_isr, &woken);

	return woken;
}

/**
 * Allocate the device structure
 */
static pios_openlrs_t pios_openlrs_alloc(void)
{
	pios_openlrs_t openlrs_dev;

	openlrs_dev = (pios_openlrs_t)PIOS_malloc(sizeof(*openlrs_dev));

	if (!openlrs_dev) {
		return NULL;
	}

	*openlrs_dev = (struct pios_openlrs_dev) {
		.magic = PIOS_OPENLRS_DEV_MAGIC,
	};

	// Create the ISR signal
	openlrs_dev->sema_isr = PIOS_Semaphore_Create();
	if (!openlrs_dev->sema_isr) {
		PIOS_free(openlrs_dev);
		return NULL;
	}

	return openlrs_dev;
}

/**
 * Validate that the device structure is valid.
 *
 * @param[in] openlrs_dev  The OpenLRS device structure pointer.
 */
static bool pios_openlrs_validate(pios_openlrs_t openlrs_dev)
{
	return openlrs_dev != NULL
		&& openlrs_dev->magic == PIOS_OPENLRS_DEV_MAGIC;
}

static void rfm22_set_channel(pios_openlrs_t openlrs_dev, uint8_t ch)
{
	/* XXX enforce maximum frequency here */
	DEBUG_PRINTF(3,"rfm22_set_channel %d\r\n", ch);
	uint8_t magicLSB = (openlrs_dev->bind_data.rf_magic & 0xff) ^ ch;
	rfm22_claim_bus(openlrs_dev);
	rfm22_write(openlrs_dev, RFM22_frequency_hopping_channel_select, openlrs_dev->bind_data.hopchannel[ch]);
	rfm22_write(openlrs_dev, RFM22_transmit_header3 + 3, magicLSB);
	rfm22_write(openlrs_dev, RFM22_check_header3 + 3, magicLSB);
	rfm22_release_bus(openlrs_dev);
}

static uint8_t rfm22_get_rssi(pios_openlrs_t openlrs_dev)
{
	return rfm22_read_claim(openlrs_dev, 0x26);
}

static void rfm22_set_modem_regs(pios_openlrs_t openlrs_dev, const struct rfm22_modem_regs *r)
{
	DEBUG_PRINTF(3,"rfm22_set_modem_regs\r\n");
	rfm22_write(openlrs_dev, RFM22_if_filter_bandwidth, r->r_1c);
	rfm22_write(openlrs_dev, RFM22_afc_loop_gearshift_override, r->r_1d);
	rfm22_write(openlrs_dev, RFM22_afc_timing_control, r->r_1e);
	rfm22_write(openlrs_dev, RFM22_clk_recovery_oversampling_ratio, r->r_20);
	rfm22_write(openlrs_dev, RFM22_clk_recovery_offset2, r->r_21);
	rfm22_write(openlrs_dev, RFM22_clk_recovery_offset1, r->r_22);
	rfm22_write(openlrs_dev, RFM22_clk_recovery_offset0, r->r_23);
	rfm22_write(openlrs_dev, RFM22_clk_recovery_timing_loop_gain1, r->r_24);
	rfm22_write(openlrs_dev, RFM22_clk_recovery_timing_loop_gain0, r->r_25);
	rfm22_write(openlrs_dev, RFM22_afc_limiter, r->r_2a);
	rfm22_write(openlrs_dev, RFM22_tx_data_rate1, r->r_6e);
	rfm22_write(openlrs_dev, RFM22_tx_data_rate0, r->r_6f);
	rfm22_write(openlrs_dev, RFM22_modulation_mode_control1, r->r_70);
	rfm22_write(openlrs_dev, RFM22_modulation_mode_control2, r->r_71);
	rfm22_write(openlrs_dev, RFM22_frequency_deviation, r->r_72);
}

static void rfm22_beacon_tone(pios_openlrs_t openlrs_dev, int16_t hz, int16_t len) //duration is now in half seconds.
{
	DEBUG_PRINTF(2,"beacon_tone: %d %d\r\n", hz, len*2);
	int16_t d = 500000 / hz; // better resolution

#if defined(PIOS_LED_LINK)
	PIOS_ANNUNC_On(PIOS_LED_LINK);
#endif /* PIOS_LED_LINK */

	if (d < 1) {
		d = 1;
	}

	rfm22_claim_bus(openlrs_dev);

	GPIO_TypeDef *gpio = openlrs_dev->cfg.spi_cfg->mosi.gpio;
	uint16_t pin_source = openlrs_dev->cfg.spi_cfg->mosi.init.GPIO_Pin;

	/* XXX Use SPI for this. */
	/* XXX this is not device independent */

#if defined(STM32F10X_MD)
	GPIO_InitTypeDef init = {
		.GPIO_Speed = GPIO_Speed_50MHz,
		.GPIO_Mode  = GPIO_Mode_Out_PP,
		.GPIO_Pin   = pin_source,
	};
#else
	GPIO_InitTypeDef init = {
		.GPIO_Speed = GPIO_Speed_50MHz,
		.GPIO_Mode  = GPIO_Mode_OUT,
		.GPIO_OType = GPIO_OType_PP,
		.GPIO_PuPd  = GPIO_PuPd_UP,
		.GPIO_Pin   = pin_source,
	};

	// Set MOSI to digital out for bit banging
	GPIO_PinAFConfig(gpio, pin_source, 0);
#endif

	GPIO_Init(gpio, &init);

	int16_t cycles = (len * 200000 / d);
	for (int16_t i = 0; i < cycles / 4; i++) {
		GPIO_SetBits(gpio, pin_source);
		PIOS_DELAY_WaituS(d);
		GPIO_ResetBits(gpio, pin_source);
		PIOS_DELAY_WaituS(d);
		GPIO_SetBits(gpio, pin_source);
		PIOS_DELAY_WaituS(d);
		GPIO_ResetBits(gpio, pin_source);
		PIOS_DELAY_WaituS(d);
		GPIO_SetBits(gpio, pin_source);
		PIOS_DELAY_WaituS(d);
		GPIO_ResetBits(gpio, pin_source);
		PIOS_DELAY_WaituS(d);
		GPIO_SetBits(gpio, pin_source);
		PIOS_DELAY_WaituS(d);
		GPIO_ResetBits(gpio, pin_source);
		PIOS_DELAY_WaituS(d);
	}

	GPIO_Init(gpio, (GPIO_InitTypeDef *) &openlrs_dev->cfg.spi_cfg->mosi.init);

#if !defined(STM32F10X_MD)
	GPIO_PinAFConfig(gpio, pin_source, openlrs_dev->cfg.spi_cfg->remap);
#endif

	rfm22_release_bus(openlrs_dev);

#if defined(PIOS_LED_LINK)
	PIOS_ANNUNC_Off(PIOS_LED_LINK);
#endif /* PIOS_LED_LINK */
}

static void rfm22_disable(pios_openlrs_t openlrs_dev)
{
	rfm22_claim_bus(openlrs_dev);

	/* disable interrupts, turn off power */
	rfm22_write(openlrs_dev, RFM22_interrupt_enable2, 0x00);
	rfm22_write(openlrs_dev, RFM22_op_and_func_ctrl1, RF22B_PWRSTATE_POWERDOWN);

	rfm22_get_it_status(openlrs_dev);

	rfm22_release_bus(openlrs_dev);
}

static void rfm22_beacon_send(pios_openlrs_t openlrs_dev, bool static_tone)
{
	DEBUG_PRINTF(2,"rfm22_beacon_send\r\n");
	rfm22_claim_bus(openlrs_dev);
	rfm22_get_it_status(openlrs_dev);
	rfm22_write(openlrs_dev, 0x06, 0x00);    // no wakeup up, lbd,
	rfm22_write(openlrs_dev, 0x07, RF22B_PWRSTATE_READY);      // disable lbd, wakeup timer, use internal 32768,xton = 1; in ready mode
	rfm22_write(openlrs_dev, 0x09, 0x7f);  // (default) c = 12.5p
	rfm22_write(openlrs_dev, 0x0a, 0x05);
	rfm22_write(openlrs_dev, 0x0b, 0x12);    // gpio0 TX State
	rfm22_write(openlrs_dev, 0x0c, 0x15);    // gpio1 RX State
	rfm22_write(openlrs_dev, 0x0d, 0xfd);    // gpio 2 micro-controller clk output
	rfm22_write(openlrs_dev, 0x0e, 0x00);    // gpio    0, 1,2 NO OTHER FUNCTION.

	rfm22_write(openlrs_dev, 0x70, 0x2C);    // disable manchest

	rfm22_write(openlrs_dev, 0x30, 0x00);    //disable packet handling

	rfm22_write(openlrs_dev, 0x79, 0);      // start channel

	rfm22_write(openlrs_dev, 0x7a, 0x05);   // 50khz step size (10khz x value) // no hopping

	rfm22_write(openlrs_dev, 0x71, 0x12);   // trclk=[00] no clock, dtmod=[01] direct using SPI, fd8=0 eninv=0 modtyp=[10] FSK
	rfm22_write(openlrs_dev, 0x72, 0x02);   // fd (frequency deviation) 2*625Hz == 1.25kHz

	rfm22_write(openlrs_dev, 0x73, 0x00);
	rfm22_write(openlrs_dev, 0x74, 0x00);    // no offset
	rfm22_release_bus(openlrs_dev);

	rfm22_set_frequency(openlrs_dev, openlrs_dev->beacon_frequency);

	rfm22_write_claim(openlrs_dev, 0x6d, 0x07);   // 7 set max power 100mW

	PIOS_Thread_Sleep(10);
	rfm22_write_claim(openlrs_dev, 0x07, RF22B_PWRSTATE_TX);        // to tx mode
	PIOS_Thread_Sleep(10);

	if (static_tone) {
		rfm22_beacon_tone(openlrs_dev, 440, 20);
	} else {
		//close encounters tune
		//  G, A, F, F(lower octave), C
		//octave 3:  392  440  349  175   261

		rfm22_beacon_tone(openlrs_dev, 392, 1);

		rfm22_write(openlrs_dev, 0x6d, 0x05);   // 5 set mid power 25mW
		PIOS_Thread_Sleep(30);
		rfm22_beacon_tone(openlrs_dev, 440,1);

		rfm22_write(openlrs_dev, 0x6d, 0x04);   // 4 set mid power 13mW
		PIOS_Thread_Sleep(30);
		rfm22_beacon_tone(openlrs_dev, 349, 1);

		rfm22_write(openlrs_dev, 0x6d, 0x02);   // 2 set min power 3mW
		PIOS_Thread_Sleep(30);
		rfm22_beacon_tone(openlrs_dev, 175,1);

		rfm22_write(openlrs_dev, 0x6d, 0x00);   // 0 set min power 1.3mW
		PIOS_Thread_Sleep(30);
		rfm22_beacon_tone(openlrs_dev, 261, 2);
	}

	rfm22_write_claim(openlrs_dev, 0x07, RF22B_PWRSTATE_READY);
}

static void rfm22_tx_packet(pios_openlrs_t openlrs_dev, void *pkt,
		uint8_t size)
{
	openlrs_dev->rf_mode = Transmit;

	rfm22_claim_bus(openlrs_dev);

	/* Clear the fifo */
	rfm22_write(openlrs_dev, RFM22_op_and_func_ctrl2, 0x03);
	rfm22_write(openlrs_dev, RFM22_op_and_func_ctrl2, 0x00);

	rfm22_write(openlrs_dev, RFM22_interrupt_enable1, RFM22_ie1_enpksent);

	rfm22_get_it_status(openlrs_dev);

	rfm22_write(openlrs_dev, RFM22_transmit_packet_length, size);   // total tx size

	rfm22_assert_cs(openlrs_dev);
	PIOS_SPI_TransferByte(openlrs_dev->spi_id, RFM22_fifo_access | 0x80);
	PIOS_SPI_TransferBlock(openlrs_dev->spi_id, pkt, NULL, size);
	rfm22_deassert_cs(openlrs_dev);

	/* Ensure sema taken */
	PIOS_Semaphore_Take(openlrs_dev->sema_isr, 0);
	/* Initiate TX */
	rfm22_write(openlrs_dev, RFM22_op_and_func_ctrl1, RF22B_PWRSTATE_TX);

	rfm22_release_bus(openlrs_dev);

	bool ok = wait_interrupt(openlrs_dev, 60000);

	rfm22_claim_bus(openlrs_dev);
	rfm22_get_it_status(openlrs_dev);
	rfm22_release_bus(openlrs_dev);

#if defined(PIOS_INCLUDE_WDG) && defined(PIOS_WDG_RFM22B)
	// Update the watchdog timer
	PIOS_WDG_UpdateFlag(PIOS_WDG_RFM22B);
#endif /* PIOS_WDG_RFM22B */

	if (!ok) {
		DEBUG_PRINTF(2,"OLRS ERR: rfm22_tx_packet timeout\r\n");
		rfm22_init(openlrs_dev, false); // reset modem
	}
}

static void rfm22_rx_packet_sizeknown(pios_openlrs_t openlrs_dev,
		void *whence, uint32_t size)
{
	/* Assumes bus claimed */

	rfm22_assert_cs(openlrs_dev);
	PIOS_SPI_TransferByte(openlrs_dev->spi_id, RFM22_fifo_access);
	PIOS_SPI_TransferBlock(openlrs_dev->spi_id, NULL,
			whence, size);
	rfm22_deassert_cs(openlrs_dev);
}

static bool rfm22_rx_packet(pios_openlrs_t openlrs_dev,
		void *whence, uint32_t size)
{
	rfm22_claim_bus(openlrs_dev);
	uint8_t len = rfm22_read(openlrs_dev, RFM22_received_packet_length);

	bool ret = false;

	if (len == size) {
		rfm22_rx_packet_sizeknown(openlrs_dev, whence, size);

		ret = true;
	}

	rfm22_release_bus(openlrs_dev);

	return ret;
}

static int rfm22_rx_packet_variable(pios_openlrs_t openlrs_dev,
		void *whence, uint32_t max_size)
{
	rfm22_claim_bus(openlrs_dev);
	uint8_t size = rfm22_read(openlrs_dev, RFM22_received_packet_length);

	int ret = -1;

	if ((size <= max_size) && (size > 0)) {
		rfm22_rx_packet_sizeknown(openlrs_dev, whence, size);

		ret = size;
	}

	rfm22_release_bus(openlrs_dev);

	return ret;
}


static void rfm22_rx_reset(pios_openlrs_t openlrs_dev, bool pause_long)
{
	openlrs_dev->rf_mode = Receive;

	DEBUG_PRINTF(3,"rfm22_rx_reset\r\n");

	if (pause_long) {
		rfm22_claim_bus(openlrs_dev);
		rfm22_write(openlrs_dev, RFM22_op_and_func_ctrl1, RF22B_PWRSTATE_READY);
		rfm22_get_it_status(openlrs_dev);
		rfm22_release_bus(openlrs_dev);
		PIOS_Thread_Sleep(20);
	}

	rfm22_claim_bus(openlrs_dev);

	/* Clear the fifo */
	rfm22_write(openlrs_dev, RFM22_op_and_func_ctrl2, 0x03);
	rfm22_write(openlrs_dev, RFM22_op_and_func_ctrl2, 0x00);

	rfm22_get_it_status(openlrs_dev);

	/* Ensure sema taken */
	PIOS_Semaphore_Take(openlrs_dev->sema_isr, 0);

	rfm22_write(openlrs_dev, RFM22_interrupt_enable1, RFM22_ie1_enpkvalid);
	rfm22_write(openlrs_dev, RFM22_op_and_func_ctrl1, RF22B_PWRSTATE_RX);   // to rx mode

	rfm22_release_bus(openlrs_dev);
}

static void rfm22_init(pios_openlrs_t openlrs_dev, uint8_t isbind)
{
	DEBUG_PRINTF(2,"rfm22_init %d\r\n", isbind);

	if (!isbind) {
		DEBUG_PRINTF(2, "Binding settings:\r\n");
		PIOS_Thread_Sleep(10);
		DEBUG_PRINTF(2, "  version: %d\r\n", openlrs_dev->bind_data.version);
		PIOS_Thread_Sleep(10);
		DEBUG_PRINTF(2, "  rf_frequency: %d\r\n", openlrs_dev->bind_data.rf_frequency);
		PIOS_Thread_Sleep(10);
		DEBUG_PRINTF(2, "  rf_power: %d\r\n", openlrs_dev->bind_data.rf_power);
		PIOS_Thread_Sleep(10);
		DEBUG_PRINTF(2, "  rf_channel_spacing: %d\r\n", openlrs_dev->bind_data.rf_channel_spacing);
		PIOS_Thread_Sleep(10);
		DEBUG_PRINTF(2, "  modem_params: %d\r\n", openlrs_dev->bind_data.modem_params);
		PIOS_Thread_Sleep(10);
		DEBUG_PRINTF(2, "  flags: %d\r\n", openlrs_dev->bind_data.flags);
		PIOS_Thread_Sleep(10);
	}


	rfm22_write_claim(openlrs_dev, RFM22_op_and_func_ctrl1, RFM22_opfc1_swres);

	PIOS_Thread_Sleep(40);

	rfm22_claim_bus(openlrs_dev);
	rfm22_write(openlrs_dev, RFM22_interrupt_enable2, 0x00);    // disable interrupts
	rfm22_write(openlrs_dev, RFM22_op_and_func_ctrl1, RF22B_PWRSTATE_READY); // disable lbd, wakeup timer, use internal 32768,xton = 1; in ready mode
	rfm22_write(openlrs_dev, RFM22_xtal_osc_load_cap, 0x7f);   // c = 12.5p
	rfm22_write(openlrs_dev, RFM22_cpu_output_clk, 0x05);
	switch (openlrs_dev->cfg.gpio_direction) {
	case GPIO0_TX_GPIO1_RX:
		rfm22_write(openlrs_dev, RFM22_gpio0_config, RFM22_gpio0_config_txstate);    // gpio0 TX State
		rfm22_write(openlrs_dev, RFM22_gpio1_config, RFM22_gpio1_config_rxstate);    // gpio1 RX State
		break;
	case GPIO0_RX_GPIO1_TX:
		rfm22_write(openlrs_dev, RFM22_gpio0_config, RFM22_gpio0_config_rxstate);    // gpio0 RX State
		rfm22_write(openlrs_dev, RFM22_gpio1_config, RFM22_gpio1_config_txstate);    // gpio1 TX State
		break;
	}
	rfm22_write(openlrs_dev, RFM22_gpio2_config, 0xfd);    // gpio 2 VDD
	rfm22_write(openlrs_dev, RFM22_io_port_config, RFM22_io_port_default);    // gpio    0, 1,2 NO OTHER FUNCTION.

	if (isbind) {
		rfm22_set_modem_regs(openlrs_dev, &modem_params[OPENLRS_BIND_PARAMS]);
	} else {
		rfm22_set_modem_regs(openlrs_dev, &modem_params[openlrs_dev->bind_data.modem_params]);
	}

	// Packet settings
	rfm22_write(openlrs_dev, RFM22_data_access_control, 0x8c);    // enable packet handler, msb first, enable crc,
	rfm22_write(openlrs_dev, RFM22_header_control1, 0x0f);    // no broadcast, check header bytes 3,2,1,0
	rfm22_write(openlrs_dev, RFM22_header_control2, 0x42);    // 4 byte header, 2 byte synch, variable pkt size
	rfm22_write(openlrs_dev, RFM22_preamble_length, (openlrs_dev->bind_data.flags & DIVERSITY_ENABLED) ? 0x14 : 0x0a);    // 40 bit preamble, 80 with diversity
	rfm22_write(openlrs_dev, RFM22_preamble_detection_ctrl1, 0x2a);    // preath = 5 (20bits), rssioff = 2
	rfm22_write(openlrs_dev, RFM22_sync_word3, 0x2d);    // synchronize word 3
	rfm22_write(openlrs_dev, RFM22_sync_word2, 0xd4);    // synchronize word 2
	rfm22_write(openlrs_dev, RFM22_sync_word1, 0x00);    // synch word 1 (not used)
	rfm22_write(openlrs_dev, RFM22_sync_word0, 0x00);    // synch word 0 (not used)

	uint32_t magic = isbind ? BIND_MAGIC : openlrs_dev->bind_data.rf_magic;
	for (uint8_t i = 0; i < 4; i++) {
		rfm22_write(openlrs_dev, RFM22_transmit_header3 + i, (magic >> 24) & 0xff);   // tx header
		rfm22_write(openlrs_dev, RFM22_check_header3 + i, (magic >> 24) & 0xff);   // rx header
		magic = magic << 8; // advance to next byte
	}

	rfm22_write(openlrs_dev, RFM22_header_enable3, 0xff);    // all the bit to be checked
	rfm22_write(openlrs_dev, RFM22_header_enable2, 0xff);    // all the bit to be checked
	rfm22_write(openlrs_dev, RFM22_header_enable1, 0xff);    // all the bit to be checked
	rfm22_write(openlrs_dev, RFM22_header_enable0, 0xff);    // all the bit to be checked

	if (isbind) {
		rfm22_write(openlrs_dev, RFM22_tx_power, BINDING_POWER);
	} else {
		rfm22_write(openlrs_dev, RFM22_tx_power, openlrs_dev->bind_data.rf_power);
	}

	rfm22_write(openlrs_dev, RFM22_frequency_hopping_step_size, openlrs_dev->bind_data.rf_channel_spacing);   // channel spacing

	rfm22_write(openlrs_dev, RFM22_frequency_offset1, 0x00);
	rfm22_write(openlrs_dev, RFM22_frequency_offset2, 0x00);    // no offset

	rfm22_release_bus(openlrs_dev);

	rfm22_rx_reset(openlrs_dev, true);

	rfm22_set_frequency(openlrs_dev,
			isbind ? binding_freq(openlrs_dev->band) :
				 openlrs_dev->bind_data.rf_frequency);
}

static void rfm22_set_frequency(pios_openlrs_t openlrs_dev, uint32_t f)
{
	/* Protect ourselves from out-of-band frequencies.  Ideally we'd latch
	 * an error here and prevent tx, but this is good enough to protect
	 * the hardware. */
	if ((f < min_freq(openlrs_dev->band)) ||
			(f > max_freq(openlrs_dev->band))) {
		f = def_carrier_freq(openlrs_dev->band);
	}

	DEBUG_PRINTF(3,"rfm22_set_frequency %d\r\n", f);
	uint16_t fb, fc, hbsel;
	if (f < 480000000) {
		hbsel = 0;
		fb = f / 10000000 - 24;
		fc = (f - (fb + 24) * 10000000) * 4 / 625;
	} else {
		hbsel = 1;
		fb = f / 20000000 - 24;
		fc = (f - (fb + 24) * 20000000) * 2 / 625;
	}
	rfm22_claim_bus(openlrs_dev);
	rfm22_write(openlrs_dev, RFM22_frequency_band_select, RFM22_fbs_sbse + (hbsel ? RFM22_fbs_hbsel : 0) + (fb & RFM22_fb_mask));
	rfm22_write(openlrs_dev, RFM22_nominal_carrier_frequency1, (fc >> 8));
	rfm22_write(openlrs_dev, RFM22_nominal_carrier_frequency0, (fc & 0xff));
	rfm22_write(openlrs_dev, RFM22_frequency_hopping_channel_select, 0);
	rfm22_release_bus(openlrs_dev);
}

/* Must have claimed bus first */
static void rfm22_get_it_status(pios_openlrs_t openlrs_dev)
{
	openlrs_dev->it_status1 = rfm22_read(openlrs_dev, RFM22_interrupt_status1);
	openlrs_dev->it_status2 = rfm22_read(openlrs_dev, RFM22_interrupt_status2);
}

static void rfm22_check_hang(pios_openlrs_t openlrs_dev)
{
	/* Hack-- detect a locked module and reset */
	if (rfm22_read_claim(openlrs_dev, 0x0C) == 0) {
		DEBUG_PRINTF(2,"OLRS ERR: RX hang\r\n");
		rfm22_init(openlrs_dev, false);
	}
}

/*****************************************************************************
* SPI Read/Write Functions
*****************************************************************************/

/**
 * Assert the chip select line.
 *
 * @param[in] rfm22b_dev  The RFM22B device.
 */
static void rfm22_assert_cs(pios_openlrs_t openlrs_dev)
{
	PIOS_DELAY_WaituS(1);
	PIOS_SPI_RC_PinSet(openlrs_dev->spi_id,
			openlrs_dev->slave_num, 0);
}

/**
 * Deassert the chip select line.
 *
 * @param[in] rfm22b_dev  The RFM22B device structure pointer.
 */
static void rfm22_deassert_cs(pios_openlrs_t openlrs_dev)
{
	PIOS_SPI_RC_PinSet(openlrs_dev->spi_id,
			openlrs_dev->slave_num, 1);
}

/**
 * Claim the SPI bus.
 *
 * @param[in] rfm22b_dev  The RFM22B device structure pointer.
 */
static void rfm22_claim_bus(pios_openlrs_t openlrs_dev)
{
	PIOS_SPI_ClaimBus(openlrs_dev->spi_id);
}

/**
 * Release the SPI bus.
 *
 * @param[in] rfm22b_dev  The RFM22B device structure pointer.
 */
static void rfm22_release_bus(pios_openlrs_t openlrs_dev)
{
	PIOS_SPI_ReleaseBus(openlrs_dev->spi_id);
}

/**
 * Write a byte to a register without claiming the bus lock
 *
 * @param[in] rfm22b_dev  The RFM22B device.
 * @param[in] addr The address to write to
 * @param[in] data The datat to write to that address
 */
static void rfm22_write(pios_openlrs_t openlrs_dev, uint8_t addr,
		uint8_t data)
{
	rfm22_assert_cs(openlrs_dev);
	uint8_t buf[2] = { addr | 0x80, data };
	PIOS_SPI_TransferBlock(openlrs_dev->spi_id, buf, NULL, sizeof(buf));
	rfm22_deassert_cs(openlrs_dev);
}

/**
 * Read a byte from an RFM22b register without claiming the bus
 *
 * @param[in] rfm22b_dev  The RFM22B device structure pointer.
 * @param[in] addr The address to read from
 * @return Returns the result of the register read
 */
static uint8_t rfm22_read(pios_openlrs_t openlrs_dev, uint8_t addr)
{
	uint8_t out[2] = { addr & 0x7F, 0xFF };
	uint8_t in[2];

	rfm22_assert_cs(openlrs_dev);
	PIOS_SPI_TransferBlock(openlrs_dev->spi_id, out, in, sizeof(out));
	rfm22_deassert_cs(openlrs_dev);

	return in[1];
}
/**
 * Claim the bus lock and write a byte to a register
 *
 * @param[in] rfm22b_dev  The RFM22B device.
 * @param[in] addr The address to write to
 * @param[in] data The datat to write to that address
 */
static void rfm22_write_claim(pios_openlrs_t openlrs_dev,
		uint8_t addr, uint8_t data)
{
	rfm22_claim_bus(openlrs_dev);
	rfm22_write(openlrs_dev, addr, data);
	rfm22_release_bus(openlrs_dev);
}

/**
 * Claim the bus lock and write a byte to a register
 *
 * @param[in] rfm22b_dev  The RFM22B device.
 * @param[in] addr The address to write to
 * @param[in] data The datat to write to that address
 */
static uint8_t rfm22_read_claim(pios_openlrs_t openlrs_dev, uint8_t addr)
{
	uint8_t ret;

	rfm22_claim_bus(openlrs_dev);
	ret = rfm22_read(openlrs_dev, addr);
	rfm22_release_bus(openlrs_dev);

	return ret;
}

#endif /* PIOS_INCLUDE_OPENLRS */

/**
 * @}
 * @}
 */
