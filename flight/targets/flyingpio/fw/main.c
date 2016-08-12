/**
 ******************************************************************************
 * @file       main.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org, Copyright (C) 2016
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


/* OpenPilot Includes */
#include <pios.h>
#include <openpilot.h>

#include "uavobjectsinit.h"
#include "systemmod.h"

#include <board_hw_defs.c>

void PIOS_Board_Init(void);

static bool inited;
static struct flyingpicmd_cfg_fa cfg;

// The other side should give us a little time to deal with this,
// as we need to copy stuff and initialize hardware.
static void handle_cfg_fa(struct flyingpicmd_cfg_fa *cmd) {
	PIOS_Assert(pios_servo_cfg.num_channels <= FPPROTO_MAX_SERVOS);

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

	inited = true;
}

static void handle_actuator_fc(struct flyingpicmd_actuator_fc *cmd) {
	PIOS_Assert(inited);

	for (int i=0; i < pios_servo_cfg.num_channels; i++) {
		PIOS_Servo_SetFraction(i, cmd->values[i],
			cfg.actuators[i].max, cfg.actuators[i].min);
	}

	PIOS_Servo_Update();

	if (cmd->led_status) {
		PIOS_LED_On(PIOS_LED_HEARTBEAT);
	} else {
		PIOS_LED_Off(PIOS_LED_HEARTBEAT);
	}
}

static void process_pio_message(void *ctx, int len, int *resp_len)
{
	(void) ctx;
	(void) len;

	*resp_len = 3;

	if (!flyingpi_calc_crc(&rx_buf, false, NULL)) {
		return;
	}

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

int main()
{
	PIOS_heap_initialize_blocks();

	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();

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
	spislave_t spislave_dev;

	tx_buf.id = 0x33;
	tx_buf.crc8 = 0x22;

	PIOS_SPISLAVE_Init(&spislave_dev, &pios_spislave_cfg, 0);

	while (1) {
		PIOS_SPISLAVE_PollSS(spislave_dev);
	}

	return 0;
}
