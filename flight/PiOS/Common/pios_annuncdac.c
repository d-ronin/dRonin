/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_ANNUNCDAC Annunciator DAC Functions
 * @brief PIOS interface for Annunciator DAC implementation
 * @{
 *
 * @file       pios_annuncdac.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
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

#if defined(PIOS_INCLUDE_DAC_ANNUNCIATOR)
#include <pios_annuncdac.h>

#include <misc_math.h>

struct annuncdac_dev_s {
	uint32_t magic;
#define ANNUNCDAC_DEV_MAGIC 0x43414441	// 'ADAC'

	dac_dev_t dac;

	volatile bool value;

	uint8_t history;
	uint16_t phase_accum;

	pios_com_callback tx_out_cb;
	uintptr_t tx_out_context;

	bool tx_running;
};

static bool PIOS_ANNUNCDAC_cb(void *ctx, uint16_t *buf, int len);

static bool PIOS_ANNUNCDAC_validate(annuncdac_dev_t annuncdac_dev)
{
	return (annuncdac_dev->magic == ANNUNCDAC_DEV_MAGIC);
}

/**
* Initialise a single beeper thingy
*/
int32_t PIOS_ANNUNCDAC_Init(annuncdac_dev_t *annuncdac_id, dac_dev_t dac)
{
	PIOS_DEBUG_Assert(annuncdac_id);
	PIOS_DEBUG_Assert(cfg);

	annuncdac_dev_t annuncdac_dev = PIOS_malloc(sizeof(*annuncdac_dev));

	if (!annuncdac_dev) return -1;

	*annuncdac_dev = (struct annuncdac_dev_s) { 
		.magic = ANNUNCDAC_DEV_MAGIC,
		.dac = dac,
	};

	*annuncdac_id = annuncdac_dev;

	PIOS_DAC_install_callback(annuncdac_dev->dac, 100, 
			PIOS_ANNUNCDAC_cb, annuncdac_dev);

	return 0;
}

static inline void FillBuf(annuncdac_dev_t dev, uint16_t *buf, int len,
		uint16_t freq, int start_mag, int stop_mag) {
	int mag_span = stop_mag - start_mag;

	for (int i=0; i < len; i++) {
		dev->phase_accum += freq;

		int magnitude = (mag_span * i + len / 2) / len;
		magnitude += start_mag;

		uint16_t result = 32768 +
	 	       	(magnitude * sin_approx(dev->phase_accum >> 1) >> 2);

		buf[i] = result & 0xFFF0;
	}
}

#define ANNUNC_FREQ 1911	/* 65536 / (24000Hz / 700Hz) =~ 1911 */
static bool PIOS_ANNUNCDAC_cb(void *ctx, uint16_t *buf, int len)
{
	annuncdac_dev_t dev = ctx;
	PIOS_Assert(PIOS_ANNUNCDAC_validate(dev));

	dev->history <<= 1;

	if (dev->value) {
		dev->history |= 1;
	}

	switch (dev->history & 3) {
		case 0:
			/* Off, has been off */
			for (int i = 0; i < len; i++) {
				buf[i] = 32768;
			}
			return true;
		case 1:
			FillBuf(dev, buf, len, ANNUNC_FREQ, 1, 6);
			break;
		case 2:
			/* Going off */
			FillBuf(dev, buf, len, ANNUNC_FREQ, 6, 1);
			break;
		case 3:
			/* Steady state */
			FillBuf(dev, buf, len, ANNUNC_FREQ, 6, 6);
			break;
	}

	return true;
}

void PIOS_ANNUNCDAC_SetValue(annuncdac_dev_t dev, bool active, bool value)
{
	(void) active;

	PIOS_Assert(PIOS_ANNUNCDAC_validate(dev));

	dev->value = value;
}

#endif /* PIOS_INCLUDE_DAC_ANNUNCIATOR */

/**
  * @}
  * @}
  */
