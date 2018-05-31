/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 *
 * @file       sparky2/fw/main.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @brief      Start up file for ChibiOS targets
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
#include "openpilot.h"

#include "misc_math.h"

#include "pios_config.h"
#include "pios_hal.h"
#include "uavobjectsinit.h"
#include "systemmod.h"
#include "pios_thread.h"

/* Function Prototypes */
extern void PIOS_Board_Init(void);
extern void Stack_Change(void);
static void initTask(void);

/**
 * @brief   Early initialization code.
 * @details This initialization must be performed just after stack setup
 *          and before any other initialization.
 */
void __early_init(void) {

  stm32_clock_init();
}

/**
 * @brief   Board-specific initialization code.
 * @todo    Add your board-specific code, if any.
 */
void boardInit(void) {
}

/**
* dRonin Main function:
*
* Initialize PiOS<BR>
* Create the "System" task (SystemModInitializein Modules/System/systemmod.c) <BR>
* Start the RTOS Scheduler<BR>
*/

int main()
{
	/* NOTE: Do NOT modify the following start-up sequence */
	PIOS_heap_initialize_blocks();

	halInit();
	chSysInit();	/* Enables interrupts */

	boardInit();

	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();
	
	initTask();

	while(1)
		PIOS_Thread_Sleep(PIOS_THREAD_TIMEOUT_MAX);

	return 0;
}

#if defined(STM32F4XX)

#include <stm32f4xx_flash.h>
/**
 * Check the brown out reset threshold is 2.7 volts and if not
 * resets it.
 */
void check_bor()
{
	uint8_t bor = FLASH_OB_GetBOR();

	if (bor != OB_BOR_LEVEL3) {
		FLASH_OB_Unlock();
		FLASH_OB_BORConfig(OB_BOR_LEVEL3);
		FLASH_OB_Launch();
		while (FLASH_WaitForLastOperation() == FLASH_BUSY) {
			;
		}
		FLASH_OB_Lock();
		while (FLASH_WaitForLastOperation() == FLASH_BUSY) {
			;
		}
	}
}

#else
void check_bor()
{
}
#endif

void system_task();

/**
 * Initialisation task.
 *
 * Runs board and module initialisation, then terminates.
 */
void initTask(void)
{
	/* Ensure BOR is programmed sane */
	check_bor();

	/* Init delay system */
	PIOS_DELAY_Init();

	/* Initialize the task monitor library */
	TaskMonitorInitialize();

	/* Initialize UAVObject libraries */
	UAVObjInitialize();

#ifndef PIPXTREME
	/* Initialize the alarms library. Reads RCC reset flags */
	AlarmsInitialize();
#endif

	uint8_t serial[PIOS_SYS_SERIAL_NUM_BINARY_LEN];
	PIOS_SYS_SerialNumberGetBinary(serial);

	for (int i = 0; i < PIOS_SYS_SERIAL_NUM_BINARY_LEN; i += 4) {
		/* TODO: consider mixing in HRNG on F4 */
		randomize_addseed(
				(serial[i]) |
				(serial[i+1] << 8) |
				(serial[i+2] << 16) |
				(serial[i+3] << 24));
	}

	PIOS_HAL_InitUAVTalkReceiver();

	/* board driver init */
	PIOS_Board_Init();

	/* Initialize modules */
	MODULE_INITIALISE_ALL(PIOS_WDG_Clear);

	/* create all modules thread */
	MODULE_TASKCREATE_ALL;

	/* Explicitly invoke system task, in this thread stack. */
	system_task();
}

/**
 * @}
 */
