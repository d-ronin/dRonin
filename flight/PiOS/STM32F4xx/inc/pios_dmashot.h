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

/**
 * @brief Initializes the DMAShot driver by loading the configuration.
 * @param[in] config Configuration struct.
 */
void PIOS_DMAShot_Init(const struct pios_dmashot_cfg *config);

/**
 * @brief Makes sure the DMAShot driver has allocated all internal structs.
 */
void PIOS_DMAShot_Prepare();

/**
 * @brief Checks whether DMAShot has been configured.
 * @retval TRUE on success, FALSE when there's no configuration.
 */
bool PIOS_DMAShot_IsConfigured();

/**
 * @brief Checks whether DMAShot is ready for use (i.e. at least one DMA configured timer).
 * @retval TRUE on success, FALSE when there's no configuration or DMA capable timers.
 */
bool PIOS_DMAShot_IsReady();

/**
 * @brief Tells the DMAShot driver about a timer that needs to be set up.
 * @param[in] timer The STM32 timer in question.
 * @param[in] clockrate The frequency the timer's set up to run.
 * @param[in] dshot_freq The desired DShot signal frequency (in KHz).
 * @retval TRUE on success, FALSE when there's no configuration for the specific timer,
                        or DMAShot isn't set up correctly.
 */
bool PIOS_DMAShot_RegisterTimer(TIM_TypeDef *timer, uint32_t clockrate, uint32_t dshot_freq);

/**
 * @brief Tells the DMAShot driver about a servo that needs to be set up.
 * @param[in] servo_channel The servo in question.
 * @retval TRUE on success, FALSE when the related timer isn't set up, or DMAShot
                        isn't set up correctly.
 */
bool PIOS_DMAShot_RegisterServo(const struct pios_tim_channel *servo_channel);

/**
 * @brief Initializes the GPIO on the registered servos for DMAShot operation.
 */
void PIOS_DMAShot_InitializeGPIOs();

/**
 * @brief Initializes and configures the registered timers for DMAShot operation.
 */
void PIOS_DMAShot_InitializeTimers(TIM_OCInitTypeDef *ocinit);

/**
 * @brief Initializes and configures the known DMA channels for DMAShot operation.
 */
void PIOS_DMAShot_InitializeDMAs();

/**
 * @brief Validates any timer and servo registrations.
 */
void PIOS_DMAShot_Validate();

/**
 * @brief Sets the throttle value of a specific servo.
 * @param[in] servo_channel The servo to update.
 * @param[in] throttle The desired throttle value (0-2047).
 * @retval TRUE on success, FALSE if the channel's not set up for DMA.
 */
void PIOS_DMAShot_WriteValue(const struct pios_tim_channel *servo_channel, uint16_t throttle);

/**
 * @brief Triggers the configured DMA channels to fire and send throttle values to the timer DMAR and optional CCRx registers.
 */
void PIOS_DMAShot_TriggerUpdate();

#endif // PIOS_DMASHOT_H
