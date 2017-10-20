/**
 ******************************************************************************
 * @file       flyingpio/fw/main.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org, Copyright (C) 2016
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup FlyingPIO FlyingPi IO Hat
 * @{
 * @brief      Start the RTOS and the Modules.
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


/* OpenPilot Includes */
#include <pios.h>
#include <openpilot.h>

#include "uavobjectsinit.h"
#include "systemmod.h"

#include <board_hw_defs.c>

#include <pios_hal.h>
#include <pios_rtc_priv.h>
#include <pios_internal_adc_simple.h>

#include "hwshared.h"

void PIOS_Board_Init(void);

static bool inited;

static struct flyingpicmd_cfg_fa cfg;

uintptr_t adc_id;
static uint16_t msg_num;

extern void TIM1_CC_IRQHandler(void);
extern void TIM1_BRK_UP_TRG_COM_IRQHandler(void);
extern void TIM2_IRQHandler(void);
extern void TIM3_IRQHandler(void);
extern void TIM14_IRQHandler(void);
extern void TIM15_IRQHandler(void);
extern void TIM16_IRQHandler(void);
extern void TIM17_IRQHandler(void);
extern void USART1_IRQHandler(void);

const void *_interrupt_vectors[USART2_IRQn] __attribute((section(".interrupt_vectors"))) = {
	[TIM1_BRK_UP_TRG_COM_IRQn] = TIM1_BRK_UP_TRG_COM_IRQHandler,
	[TIM1_CC_IRQn] = TIM1_CC_IRQHandler,
	[TIM3_IRQn] = TIM3_IRQHandler,
	[TIM14_IRQn] = TIM14_IRQHandler,
	[TIM15_IRQn] = TIM15_IRQHandler,
	[TIM16_IRQn] = TIM16_IRQHandler,
	[TIM17_IRQn] = TIM17_IRQHandler,
	[USART1_IRQn] = USART1_IRQHandler,
};

// The other side should give us a little time to deal with this,
// as we need to copy stuff and initialize hardware.
static void handle_cfg_fa(struct flyingpicmd_cfg_fa *cmd) {
	PIOS_Assert(pios_servo_cfg.num_channels <= FPPROTO_MAX_SERVOS);

	if (cfg.receiver_protocol != cmd->receiver_protocol) {
		// This is not supposed to change types.  Assert, which will
		// trigger watchdog, which will trigger picking up new type.
		PIOS_Assert(!inited);

		PIOS_HAL_ConfigurePort(cmd->receiver_protocol,
			&pios_usart_rcvr_cfg,
			&pios_usart_com_driver,
			NULL,			// I2C
			NULL,
			&pios_ppm_cfg,
			NULL,			// PWM
			PIOS_LED_HEARTBEAT,	// It's all we have...
			&pios_dsm_rcvr_cfg,
			HWSHARED_DSMXMODE_AUTODETECT,
			NULL			// sbus: we have inverters
		);
	}

	// Copy so that we can refer to this later.
	memcpy(&cfg, cmd, sizeof(cfg));

	uint16_t minimums[pios_servo_cfg.num_channels];
	uint16_t maximums[pios_servo_cfg.num_channels];

	// Annoying reformat..
	for (int i=0; i < pios_servo_cfg.num_channels; i++) {
		minimums[i] = cfg.actuators[i].min;
		maximums[i] = cfg.actuators[i].max;
	}

	PIOS_Servo_SetMode(cfg.rate, FPPROTO_MAX_BANKS, maximums, minimums);

	/* OK, game on.  From here on out we may end up generating PWM.
	 * so it's time to enable the watchdog. */
	if (!inited) {
		PIOS_WDG_Init();
		inited = true;
	}
}

static void handle_actuator_fc(struct flyingpicmd_actuator_fc *cmd) {
	if (!inited) {
		/* Can happen after a watchdog reset... In which case,
		 * wait for the upstream processor to tll us it's OK
		 */
		return;
	}

	for (int i=0; i < pios_servo_cfg.num_channels; i++) {
		PIOS_Servo_SetFraction(i, cmd->values[i],
			cfg.actuators[i].max, cfg.actuators[i].min);
	}

	PIOS_Servo_Update();

	if (cmd->led_status) {
		PIOS_ANNUNC_On(PIOS_LED_HEARTBEAT);
	} else {
		PIOS_ANNUNC_Off(PIOS_LED_HEARTBEAT);
	}

	PIOS_WDG_Clear();
}

static void generate_status_message(int *resp_len)
{
	tx_buf.id = FLYINGPIRESP_IO;

	struct flyingpiresp_io_10 *resp = &tx_buf.body.io_10;

	bzero(resp, sizeof(*resp));

	resp->valid_messages_recvd = msg_num;

	for (int i=0; i<FPPROTO_MAX_RCCHANS; i++) {
		if (pios_rcvr_group_map[0]) {
			resp->chan_data[i] =
				/* 1-offset vs 0-offset grr */
				PIOS_RCVR_Read(pios_rcvr_group_map[0], i+1);
		} else {
			resp->chan_data[i] = PIOS_RCVR_NODRIVER;
		}
	}

	for (int i=0; i<FPPROTO_MAX_ADCCHANS; i++) {
		resp->adc_data[i] = PIOS_ADC_DevicePinGet(adc_id, i);
	}

	flyingpi_calc_crc(&tx_buf, true, resp_len);

	msg_num++;
}

static void process_pio_message_impl(int *resp_len)
{
	switch (rx_buf.id) {
		case FLYINGPICMD_ACTUATOR:
			handle_actuator_fc(&rx_buf.body.actuator_fc);
			break;
		case FLYINGPICMD_CFG:
			handle_cfg_fa(&rx_buf.body.cfg_fa);
			break;
		default:
			/* We got a message with an unknown type, but valid
			 * CRC.  no bueno. */
			PIOS_Assert(0);
			break;
	}

	/* If we get an edge and no clocking.. make sure the message is
	 * invalidated / not reused */
	rx_buf.id = 0;
}

static void process_pio_message(void *ctx, int len, int *resp_len)
{
	(void) ctx;
	(void) len;

	if (flyingpi_calc_crc(&rx_buf, false, NULL)) {
		process_pio_message_impl(resp_len);
	}

	generate_status_message(resp_len);
}

int main()
{
	PIOS_heap_initialize_blocks();

	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();

#if defined(PIOS_INCLUDE_ANNUNC)
	const struct pios_annunc_cfg *led_cfg = PIOS_BOARD_HW_DEFS_GetLedCfg(0);
	PIOS_Assert(led_cfg);
	PIOS_ANNUNC_Init(led_cfg);
#endif	/* PIOS_INCLUDE_ANNUNC */

#if defined(PIOS_INCLUDE_ADC)
	pios_internal_adc_t adc_dev;

	if (PIOS_INTERNAL_ADC_Init(&adc_dev, &internal_adc_cfg)) {
		PIOS_Assert(0);
	}

	if (PIOS_ADC_Init(&adc_id, &pios_internal_adc_driver, (uintptr_t )adc_dev)) {
		PIOS_Assert(0);
	}
#endif

	//outputs
	PIOS_TIM_InitClock(&tim_1_cfg);
	PIOS_TIM_InitClock(&tim_3_cfg);
	PIOS_TIM_InitClock(&tim_14_cfg);

	PIOS_Servo_Init(&pios_servo_cfg);
	spislave_t spislave_dev;

	tx_buf.id = 0x33;
	tx_buf.crc8 = 0x22;

	int initial_msg_len;
	generate_status_message(&initial_msg_len);

	PIOS_SPISLAVE_Init(&spislave_dev, &pios_spislave_cfg, initial_msg_len);

	uint32_t i=0;

	while (1) {
		PIOS_SPISLAVE_PollSS(spislave_dev);
		PIOS_INTERNAL_ADC_DoStep(adc_dev);

		i++;
		i &= 0x3ffff;

		if (!inited) {
			// Distinctive "ready" blip when host is not controlling
			// LED.
			if ((i == 0xc000) || (i == 0) || (i==0x18000)) {
				PIOS_ANNUNC_Toggle(PIOS_LED_HEARTBEAT);
			}
		}
	}

	return 0;
}

/**
 * @}
 * @}
 */
