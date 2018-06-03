/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_ADC ADC Functions
 * @{
 *
 * @file       pios_internal_adc.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @brief      STM32F30x internal ADC PIOS interface
 * @see        The GNU Public License (GPL) Version 3
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

#include <pios_internal_adc_simple.h>

#if defined(PIOS_INCLUDE_ADC)

static void PIOS_INTERNAL_ADC_PinConfig(pios_internal_adc_t adc);
static void PIOS_INTERNAL_ADC_Converter_Config(pios_internal_adc_t adc);
static int32_t PIOS_INTERNAL_ADC_PinGet(uintptr_t internal_adc_id, uint32_t pin);
static uint8_t PIOS_INTERNAL_ADC_NumberOfChannels(uintptr_t internal_adc_id);
static float PIOS_INTERNAL_ADC_LSB_Voltage(uintptr_t internal_adc_id);

const struct pios_adc_driver pios_internal_adc_driver = {
		.get_pin = PIOS_INTERNAL_ADC_PinGet,
		.number_of_channels = PIOS_INTERNAL_ADC_NumberOfChannels,
		.lsb_voltage = PIOS_INTERNAL_ADC_LSB_Voltage,
};

#define MAX_CHANNELS 8

struct pios_internal_adc_dev {
	const struct pios_internal_adc_simple_cfg * cfg;

	uint8_t conversion_started : 1;
	uint8_t current_pin : 3;

	volatile uint16_t pin_values[MAX_CHANNELS];
};

/**
 * @brief Validates an internal ADC device
 * \return true if device is valid
 */
static bool PIOS_INTERNAL_ADC_validate(pios_internal_adc_t dev)
{
	if (dev == NULL )
		return false;

	return true;
}

/**
 * @brief Allocates an internal ADC device
 */
static pios_internal_adc_t PIOS_INTERNAL_ADC_Allocate()
{
	pios_internal_adc_t adc_dev = PIOS_malloc(sizeof(*adc_dev));
	if (!adc_dev)
		return (NULL);

	return (adc_dev);
}

/**
 * @brief Configures the pins used on the ADC device
 * \param[in] handle to the ADC device
 */
static void PIOS_INTERNAL_ADC_PinConfig(pios_internal_adc_t adc_dev)
{
	if (!PIOS_INTERNAL_ADC_validate(adc_dev)) {
		return;
	}
	/* Setup analog pins */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;

	for (int32_t i = 0; i < adc_dev->cfg->adc_pin_count; i++) {
		adc_dev->pin_values[i] = 0xffff;

		if (adc_dev->cfg->adc_pins[i].port == NULL )
			continue;
		GPIO_InitStructure.GPIO_Pin = adc_dev->cfg->adc_pins[i].pin;
		GPIO_Init(adc_dev->cfg->adc_pins[i].port, (GPIO_InitTypeDef*) &GPIO_InitStructure);
	}
}

/**
 * @brief Configures the ADC device
 * \param[in] handle to the ADC device
 */
static void PIOS_INTERNAL_ADC_Converter_Config(pios_internal_adc_t adc_dev)
{
	ADC_DeInit(adc_dev->cfg->adc_dev);

	/* Perform ADC calibration */
	ADC1->CR |= ADC_CR_ADCAL;
	while ((ADC1->CR & ADC_CR_ADCAL) != 0);

	ADC_InitTypeDef ADC_InitStructure;
	ADC_StructInit(&ADC_InitStructure);

	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;

	ADC_Init(adc_dev->cfg->adc_dev, &ADC_InitStructure);

	ADC_ClockModeConfig(adc_dev->cfg->adc_dev, ADC_ClockMode_AsynClk);
	ADC_ContinuousModeCmd(adc_dev->cfg->adc_dev, DISABLE);
	ADC_DiscModeCmd(adc_dev->cfg->adc_dev, ENABLE);

	/* Do common ADC init */
	ADC_TempSensorCmd(ENABLE);
	ADC_VrefintCmd(ENABLE);

	ADC_Cmd(adc_dev->cfg->adc_dev, ENABLE);

	while (!ADC_GetFlagStatus(adc_dev->cfg->adc_dev, ADC_FLAG_ADRDY));
}

/**
 * @brief Init the ADC.
 */
int32_t PIOS_INTERNAL_ADC_Init(pios_internal_adc_t * internal_adc_id, const struct pios_internal_adc_simple_cfg * cfg)
{
	PIOS_DEBUG_Assert(internal_adc_id); PIOS_DEBUG_Assert(cfg);

	PIOS_Assert(cfg->adc_pin_count <= MAX_CHANNELS);

	pios_internal_adc_t adc_dev;
	adc_dev = PIOS_INTERNAL_ADC_Allocate();
	if (adc_dev == NULL )
		return -1;
	adc_dev->cfg = cfg;

	*internal_adc_id = adc_dev;

	adc_dev->current_pin = 0;

	PIOS_INTERNAL_ADC_PinConfig(adc_dev);

	PIOS_INTERNAL_ADC_Converter_Config(adc_dev);
	return 0;
}

void PIOS_INTERNAL_ADC_DoStep(pios_internal_adc_t adc_dev)
{
	if (!adc_dev->conversion_started) {
		/* Read the DR to ensure that the ADC_FLAG_EOC flag is not set */
		ADC_GetConversionValue(adc_dev->cfg->adc_dev);


		adc_dev->cfg->adc_dev->CHSELR = adc_dev->cfg->adc_pins[adc_dev->current_pin].adc_channel;
		adc_dev->cfg->adc_dev->SMPR = ADC_SampleTime_41_5Cycles;
		/* 41.5 cycles is a very long integration time, but we're in
		 * no rush */

		ADC_StartOfConversion(adc_dev->cfg->adc_dev);

		adc_dev->conversion_started = 1;
	} else {
		if (ADC_GetFlagStatus(adc_dev->cfg->adc_dev,
				ADC_FLAG_ADSTART) == RESET) {
			adc_dev->pin_values[adc_dev->current_pin] =
				ADC_GetConversionValue(adc_dev->cfg->adc_dev);

			adc_dev->current_pin++;

			if (adc_dev->current_pin > adc_dev->cfg->adc_pin_count) {
				adc_dev->current_pin = 0;
			}

			adc_dev->conversion_started = 0;
		}
	}	
}

/**
 * Checks the number of channels on a certain ADC device
 * @param[in] internal_adc_id handler to the device to check
 * @return number of channels on the device.
 */
static uint8_t PIOS_INTERNAL_ADC_NumberOfChannels(uintptr_t internal_adc_id)
{
	pios_internal_adc_t adc_dev = (pios_internal_adc_t) internal_adc_id;
	if (!PIOS_INTERNAL_ADC_validate(adc_dev))
		return 0;
	return adc_dev->cfg->adc_pin_count;
}

/**
 * @brief Gets the value of an ADC pinn
 * @param[in] pin number, acording to the order passed on the configuration
 * @return ADC pin value averaged over the set of samples since the last reading.
 * @return -1 if pin doesn't exist
 */
static int32_t PIOS_INTERNAL_ADC_PinGet(uintptr_t internal_adc_id, uint32_t pin)
{
	pios_internal_adc_t adc_dev = (pios_internal_adc_t) internal_adc_id;

	/* Check if pin exists */
	if (pin >= adc_dev->cfg->adc_pin_count) {
		return -1;
	}

	return adc_dev->pin_values[pin];
}

/**
 * @brief Gets the least significant bit voltage of the ADC
 */
static float PIOS_INTERNAL_ADC_LSB_Voltage(uintptr_t internal_adc_id)
{
	pios_internal_adc_t adc_dev = (pios_internal_adc_t) internal_adc_id;
	if (!PIOS_INTERNAL_ADC_validate(adc_dev)) {
		return 0;
	}

	// Though we have 12 bits of resolution, it's left-adjusted into a 16
	// bit word.
        return VREF_PLUS / (((uint32_t)1 << 16) - 16);
}
#endif /* PIOS_INCLUDE_ADC */
/** 
 * @}
 * @}
 */
