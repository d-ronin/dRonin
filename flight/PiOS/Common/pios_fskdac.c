/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_FSKDAC FSK DAC Functions
 * @brief PIOS interface for FSK DAC implementation
 * @{
 *
 * @file       pios_fskdac.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015-2016
 * @brief      Generates Bel202 encoded serial data on the DAC channel
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* Project Includes */
#include <pios.h>

#if defined(PIOS_INCLUDE_DAC_FSK)
#include <pios_fskdac.h>

#include <misc_math.h>

/* Provide a COM driver */
static void PIOS_FSKDAC_RegisterTxCallback(uintptr_t fskdac_id, pios_com_callback tx_out_cb, uintptr_t context);
static void PIOS_FSKDAC_TxStart(uintptr_t fskdac_id, uint16_t tx_bytes_avail);

#define SAMPLES_PER_BIT 20

const struct pios_com_driver pios_fskdac_com_driver = {
	.tx_start   = PIOS_FSKDAC_TxStart,
	.bind_tx_cb = PIOS_FSKDAC_RegisterTxCallback,
};

struct fskdac_dev_s {
	uint32_t magic;
#define FSKDAC_DEV_MAGIC 0x674b5346	// 'FSKg'

	dac_dev_t dac;

	// Right now assumes we are at the start of a bit -- that there are an
	// integral number of bits per sample buffer.  at 24KHz, 20 samples
	// per bit.

	uint16_t send_state;
	uint16_t phase_accum;

	pios_com_callback tx_out_cb;
	uintptr_t tx_out_context;

	bool tx_running;
};

static bool PIOS_FSKDAC_cb(void *ctx, uint16_t *buf, int len);

static bool PIOS_FSKDAC_validate(fskdac_dev_t fskdac_dev)
{
	return (fskdac_dev->magic == FSKDAC_DEV_MAGIC);
}

/**
* Initialise a single USART device
*/
int32_t PIOS_FSKDAC_Init(fskdac_dev_t *fskdac_id, dac_dev_t dac)
{
	PIOS_DEBUG_Assert(fskdac_id);
	PIOS_DEBUG_Assert(cfg);

	fskdac_dev_t fskdac_dev = PIOS_malloc(sizeof(*fskdac_dev));

	if (!fskdac_dev) return -1;

	*fskdac_dev = (struct fskdac_dev_s) { 
		.magic = FSKDAC_DEV_MAGIC,
		.dac = dac,
	};

	*fskdac_id = fskdac_dev;

	return 0;
}

static void PIOS_FSKDAC_RegisterTxCallback(uintptr_t fskdac_id, pios_com_callback tx_out_cb, uintptr_t context)
{
	fskdac_dev_t fskdac_dev = (fskdac_dev_t) fskdac_id;

	bool valid = PIOS_FSKDAC_validate(fskdac_dev);
	PIOS_Assert(valid);

	/* 
	 * Order is important in these assignments since ISR uses _cb
	 * field to determine if it's ok to dereference _cb and _context
	 */
	/* XXX this is not safe */
	fskdac_dev->tx_out_context = context;
	fskdac_dev->tx_out_cb = tx_out_cb;
}

static void PIOS_FSKDAC_TxStart(uintptr_t fskdac_id, uint16_t tx_bytes_avail)
{
	fskdac_dev_t fskdac_dev = (fskdac_dev_t)fskdac_id;

	bool valid = PIOS_FSKDAC_validate(fskdac_dev);
	PIOS_Assert(valid);

	if (fskdac_dev->tx_running) {
		return;
	}

	fskdac_dev->tx_running = true;

	PIOS_DAC_install_callback(fskdac_dev->dac, 100, 
			PIOS_FSKDAC_cb, fskdac_dev);
}

// TODO Consider relocation to common header
static inline uint16_t serial_frame_char(uint8_t c, int bits, bool parity, bool even,
		bool twostop, bool inverted, uint8_t *framed_size)
{
	// Bits is expected to be 7 or 8 currently
	int num = 2 + bits;		// Single start and stop counted.

	// Put in character.  Start bit is low on the left, which we need to leave
	// in at the end.  Leave one more bit's room.
	uint16_t framed = c << 1;

	if (bits == 7) {
		// If we're 7 bit, and the high bit is set, don't let it crash into
		// the start bit.
		framed &= 0xff;
	}

	if (parity) {
		num++;
		if (!even) {
			framed |= 1;
		}

		for (int i = 0; i < bits; i++) {
			framed ^= c & 1;
			c >>= 1;
		}

		framed <<= 1;		// Make room for stop
	}

	if (twostop) {
		num++;

		framed <<= 1;		// Make room for add'l stop

		framed |= 3;
	} else {
		framed |= 1;
	}

	if (inverted) {
		framed = ~framed;
	}

	// 7N1 = 2 + 7 = 9
	// 8E2 = 2 + 9 + 1 + 1 = 12
	
	framed <<= (16 - num);

	if (framed_size) {
		*framed_size = num;
	}

	return framed;
}

static bool PIOS_FSKDAC_cb(void *ctx, uint16_t *buf, int len)
{
	fskdac_dev_t dev = ctx;

	bool no_next_c = false;

	for (int i = 0; i < len; i += SAMPLES_PER_BIT) {
		if ((!no_next_c) && (!dev->send_state)) {
			uint8_t b;
			uint16_t bytes_to_send = 0;

			bool tx_need_yield;

			if (dev->tx_out_cb) {
				bytes_to_send = (dev->tx_out_cb)(dev->tx_out_context, 
						&b, 1, NULL, &tx_need_yield);
			}

			// pack into a reasonable form 8 / O / 1
			if (bytes_to_send > 0) {
				dev->send_state = serial_frame_char(b,
						8, true, false, false,
						false, NULL);
			} else {
				no_next_c = true;
			}
		}

		uint16_t freq = ((dev->send_state & 0x8000) || (no_next_c)) ?
			3277 : 6008;		/* 1200 mark / 2200 space */

		for (int j = 0; j < SAMPLES_PER_BIT; j++) {
			dev->phase_accum += freq;

			uint16_t result = 32768 +
				((sin_approx(dev->phase_accum >> 1) * 6) >> 2);

			buf[i + j] = result & 0xFFF0;
		}

		dev->send_state <<= 1;
	}

	return true;
}

#endif /* PIOS_INCLUDE_DAC_FSK */

/**
  * @}
  * @}
  */
