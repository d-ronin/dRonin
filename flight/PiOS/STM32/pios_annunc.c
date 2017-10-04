/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_ANNUNC
 * @brief Annunciator functions-- Annunciators, buzzers, etc.
 * @{
 *
 * @file       pios_annunc.c   
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Annunciator raw functions, init, on & off.
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

/* Project Includes */
#include "pios.h"

#if defined(PIOS_INCLUDE_ANNUNC)

#include <pios_annunc_priv.h>

const static struct pios_annunc_cfg * annunciator_cfg;

#define MAXANNUNC 8

static uint8_t annunc_state;

static bool get_annunc_state(int idx) {
	if (annunc_state & (1 << idx)) {
		return true;
	} else {
		return false;
	}
}

static void set_annunc_state(int idx, bool active) {
	uint8_t mask = 1 << idx;
	uint8_t val = 0;

	if (active) {
		val = mask;
	}

	annunc_state = (annunc_state & (~mask)) | val;
}

/**
 * Initialises all the Annunciators
 */
int32_t PIOS_ANNUNC_Init(const struct pios_annunc_cfg * cfg)
{
	PIOS_Assert(cfg);
	
	/* Store away the config in a global used by API functions */
	annunciator_cfg = cfg;

	PIOS_Assert(cfg->num_annunciators <= MAXANNUNC);
	
	for (uint8_t i = 0; i < cfg->num_annunciators; i++) {
		const struct pios_annunc * annunciator = &(cfg->annunciators[i]);
		
		if (annunciator->remap) {
#ifdef STM32F10X_MD
			GPIO_PinRemapConfig(annunciator->remap, ENABLE);
#else
			GPIO_PinAFConfig(annunciator->pin.gpio, annunciator->pin.init.GPIO_Pin, annunciator->remap);
#endif

		}
		
		GPIO_Init(annunciator->pin.gpio, (GPIO_InitTypeDef*)&annunciator->pin.init);
		
		PIOS_ANNUNC_Off(i);
	}
	
	return 0;
}

/**
 * Turn on Annunciator
 * \param[in] Annunciator Annunciator id
 */
void PIOS_ANNUNC_On(uint32_t annunc_id)
{
	PIOS_Assert(annunciator_cfg);
	
	if (annunc_id >= annunciator_cfg->num_annunciators) {
		/* Annunciator index out of range */
		return;
	}
	
	const struct pios_annunc * annunciator =
		&(annunciator_cfg->annunciators[annunc_id]);

	set_annunc_state(annunc_id, true);
	
	if (annunciator->handler) {
		annunciator->handler(true);
		return;
	}

	if (!annunciator->pin.gpio) {
		return;
	}

	if (annunciator->active_high) {
		GPIO_SetBits(annunciator->pin.gpio,
				annunciator->pin.init.GPIO_Pin);
	} else {
		GPIO_ResetBits(annunciator->pin.gpio,
				annunciator->pin.init.GPIO_Pin);
	}
}

/**
 * Turn off Annunciator
 * \param[in] Annunciator Annunciator id
 */
void PIOS_ANNUNC_Off(uint32_t annunc_id)
{
	PIOS_Assert(annunciator_cfg);

	if (annunc_id >= annunciator_cfg->num_annunciators) {
		/* Annunciator index out of range */
		return;
	}
	
	const struct pios_annunc * annunciator =
		&(annunciator_cfg->annunciators[annunc_id]);
	
	set_annunc_state(annunc_id, false);

	if (annunciator->handler) {
		annunciator->handler(false);
		return;
	}

	if (!annunciator->pin.gpio) {
		return;
	}

	if (annunciator->active_high) {
		GPIO_ResetBits(annunciator->pin.gpio,
				annunciator->pin.init.GPIO_Pin);
	} else {
		GPIO_SetBits(annunciator->pin.gpio,
				annunciator->pin.init.GPIO_Pin);
	}
}

/**
 * Toggle Annunciator on/off
 * \param[in] Annunciator Annunciator id
 * DEPRECATED.
 */
void PIOS_ANNUNC_Toggle(uint32_t annunc_id)
{
	PIOS_Assert(annunciator_cfg);
	
	if (annunc_id >= annunciator_cfg->num_annunciators) {
		/* Annunciator index out of range */
		return;
	}

	if (get_annunc_state(annunc_id)) {
		PIOS_ANNUNC_Off(annunc_id);
	} else {
		PIOS_ANNUNC_On(annunc_id);
	}
}

#endif

/**
 * @}
 * @}
 */
