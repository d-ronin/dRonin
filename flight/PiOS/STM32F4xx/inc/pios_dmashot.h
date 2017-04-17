/**
 ******************************************************************************
 * @file       pios_dmashot.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DMAShot PiOS DMA-driven DShot driver
 * @{
 * @brief Generates DShot signal by updating timer CC registers via DMA bursts.
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef PIOS_DMASHOT_H
#define PIOS_DMASHOT_H

#include <stdint.h>

#include <pios_stm32.h>
#include "stm32f4xx_tim.h"

#include "pios_tim_priv.h"

// Frequencies.
#define DMASHOT_150                                             150000
#define DMASHOT_300                                             300000
#define DMASHOT_600                                             600000
#define DMASHOT_1200                                            1200000

/**
 * @brief Configuration struct to assign a DMA channel and stream to a timer, and
                        optionally specify a master timer to update single timer registers of
                        timers without DMA channel.
 */
struct pios_dmashot_timer_cfg {

	TIM_TypeDef *timer;
	DMA_Stream_TypeDef *stream;
	uint32_t channel;
	uint32_t tcif;

	TIM_TypeDef *master_timer;
	uint16_t master_config;

};

/**
 * @brief Configuration struct holding all timer configurations.
 */
struct pios_dmashot_cfg {

	const struct pios_dmashot_timer_cfg *timer_cfg;
	uint8_t num_timers;

};

void PIOS_DMAShot_Init(const struct pios_dmashot_cfg *config);
void PIOS_DMAShot_Prepare();
bool PIOS_DMAShot_IsConfigured();
bool PIOS_DMAShot_IsReady();

bool PIOS_DMAShot_RegisterTimer(TIM_TypeDef *timer, uint32_t clockrate, uint32_t dshot_freq);
bool PIOS_DMAShot_RegisterServo(const struct pios_tim_channel *servo_channel);

void PIOS_DMAShot_InitializeGPIOs();
void PIOS_DMAShot_InitializeTimers(TIM_OCInitTypeDef *ocinit);
void PIOS_DMAShot_InitializeDMAs();
void PIOS_DMAShot_Validate();

bool PIOS_DMAShot_WriteValue(const struct pios_tim_channel *servo_channel, uint16_t throttle);

void PIOS_DMAShot_TriggerUpdate();

#endif // PIOS_DMASHOT_H
