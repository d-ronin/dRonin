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

void process_pio_message(void *ctx, int len, int *resp_len)
{
	(void) ctx;
	(void) len;

	*resp_len = 3;

	if (!flyingpi_calc_crc(&rx_buf, false, NULL)) {
		return;
	}

	if (rx_buf.id == FLYINGPICMD_ACTUATOR) {
		if (rx_buf.body.actuator_fc.led_status) {
			PIOS_LED_On(PIOS_LED_HEARTBEAT);
		} else {
			PIOS_LED_Off(PIOS_LED_HEARTBEAT);
		}
	}
	/* If we get an edge and no clocking.. make sure the message is
	 * invalidated */
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
