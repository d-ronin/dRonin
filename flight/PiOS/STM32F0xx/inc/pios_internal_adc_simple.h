/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_INTERNAL_ADC Internal ADC Functions
 * @{
 *
 * @file       pios_internal_adc_priv.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @brief      ADC private definitions.
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

#ifndef PIOS_INTERNAL_ADC_PRIV_H
#define PIOS_INTERNAL_ADC_PRIV_H

#include <pios.h>
#include <pios_stm32.h>

#include <stm32f0xx_adc.h>

extern const struct pios_adc_driver pios_internal_adc_driver;

typedef struct pios_internal_adc_dev *pios_internal_adc_t;

/**
 * A structure that defines an internal ADC pin
 */
struct adc_pin {
	GPIO_TypeDef *port; 	/**< port of the pin ex:GPIOC */
	uint32_t pin; 		/**< port pin number ex:GPIO_Pin_3 */
	uint32_t adc_channel; 	/**< adc channel corresponding to the pin ex:ADC_Channel_9 */
};

/**
 * A structure that defines an internal ADC configuration
 */
struct pios_internal_adc_simple_cfg {
	ADC_TypeDef* adc_dev;	/**< ADC periph ex:ADC1 */

	uint8_t adc_pin_count;	/**< Number of ADC pins to use */
	struct adc_pin adc_pins[];	/**< Array of pins to use */
};

int32_t PIOS_INTERNAL_ADC_Init(pios_internal_adc_t *internal_adc_id, const struct pios_internal_adc_simple_cfg * cfg);

void PIOS_INTERNAL_ADC_DoStep(pios_internal_adc_t internal_adc_id);

#endif /* PIOS_INTERNAL_ADC_PRIV_H */

/**
 * @}
 * @}
 */

