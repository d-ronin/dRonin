/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup Sparky Tau Labs Sparky support files
 * @{
 *
 * @file       pios_board.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @brief      The board specific initialization routines
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

/* Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */

#include "board_hw_defs.c"

#include <pios.h>
//#include <pios_hal.h>
#include <openpilot.h>
#include <uavobjectsinit.h>
#include "hwsparky.h"
#include "manualcontrolsettings.h"
#include "modulesettings.h"

/**
 * Configuration for the MS5611 chip
 */
#if defined(PIOS_INCLUDE_MS5611)
#include "pios_ms5611_priv.h"
static const struct pios_ms5611_cfg pios_ms5611_cfg = {
	.oversampling = MS5611_OSR_1024,
	.temperature_interleaving = 1,
};
#endif /* PIOS_INCLUDE_MS5611 */

/**
 * Configuration for the external HMC5883 chip
 */
#if defined(PIOS_INCLUDE_HMC5883)
#include "pios_hmc5883_priv.h"
static const struct pios_hmc5883_cfg pios_hmc5883_external_cfg = {
	.exti_cfg            = NULL,
	.M_ODR               = PIOS_HMC5883_ODR_75,
	.Meas_Conf           = PIOS_HMC5883_MEASCONF_NORMAL,
	.Gain                = PIOS_HMC5883_GAIN_1_9,
	.Mode                = PIOS_HMC5883_MODE_SINGLE,
	.Default_Orientation = PIOS_HMC5883_TOP_0DEG,
};
#endif /* PIOS_INCLUDE_HMC5883 */

bool external_mag_fail;

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

#include <pios_board_info.h>

void PIOS_Board_Init(void)
{
	/* Delay system */
	//PIOS_DELAY_Init();

	const struct pios_board_info *bdinfo = &pios_board_info_blob;

#if defined(PIOS_INCLUDE_LED)
	const struct pios_led_cfg *led_cfg = PIOS_BOARD_HW_DEFS_GetLedCfg(bdinfo->board_rev);
	PIOS_Assert(led_cfg);
	PIOS_LED_Init(led_cfg);
#endif	/* PIOS_INCLUDE_LED */

#if defined(PIOS_INCLUDE_RTC)
	/* Initialize the real-time clock and its associated tick */
	PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif

	//outputs
	PIOS_TIM_InitClock(&tim_1_cfg);
	PIOS_TIM_InitClock(&tim_3_cfg);
	PIOS_TIM_InitClock(&tim_14_cfg);

	PIOS_Servo_Init(&pios_servo_cfg);

#if 0
#if defined(PIOS_INCLUDE_ADC)
	if (number_of_adc_ports > 0) {
		internal_adc_cfg.adc_pin_count = number_of_adc_ports;
		uint32_t internal_adc_id;
		if (PIOS_INTERNAL_ADC_Init(&internal_adc_id, &internal_adc_cfg) < 0)
			PIOS_Assert(0);
		PIOS_ADC_Init(&pios_internal_adc_id, &pios_internal_adc_driver, internal_adc_id);
	}
#endif /* PIOS_INCLUDE_ADC */

#if defined(PIOS_INCLUDE_GPIO)
	PIOS_GPIO_Init();
#endif
#endif
}

/**
 * @}
 */
