/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup PipX PipXtreme Radio
 * @{
 *
 * @file       pipxtreme/fw/main.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      Start FreeRTOS and the Modules.
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
#include "uavobjectsinit.h"
#include "systemmod.h"
#include "pios_thread.h"

/* Global Variables */

/* Prototype of PIOS_Board_Init() function */
extern void PIOS_Board_Init(void);
extern void Stack_Change(void);

/* Local Variables */
#define INIT_TASK_PRIORITY	PIOS_THREAD_PRIO_HIGHEST
#define INIT_TASK_STACK		1024
static struct pios_thread *initTaskHandle;

/* Function Prototypes */
static void initTask(void *parameters);

/* Prototype of generated InitModules() function */
extern void InitModules(void);

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

/**
* OpenPilot Main function:
*
* Initialize PiOS<BR>
* Create the "System" task (SystemModInitializein Modules/System/systemmod.c) <BR>
* Start RTOS Scheduler <BR>
* If something goes wrong, blink LED1 and LED2 every 100ms
*
*/
int main()
{
	/* NOTE: Do NOT modify the following start-up sequence */
	PIOS_heap_initialize_blocks();

	halInit();
	chSysInit();
	boardInit();

	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();
	/* Delay system */
	PIOS_DELAY_Init();

	/* We use a task to bring up the system so we can */
	/* always rely on RTOS primitives */
	initTaskHandle = PIOS_Thread_Create(initTask, "init", INIT_TASK_STACK, NULL, INIT_TASK_PRIORITY);
	PIOS_Assert(initTaskHandle != NULL);

	while (true) {
		PIOS_Thread_Sleep(PIOS_THREAD_TIMEOUT_MAX);
	}

	return 0;
}
/**
 * Initialisation task.
 *
 * Runs board and module initialisation, then terminates.
 */
void
initTask(void *parameters)
{
	/* board driver init */
	PIOS_Board_Init();

	/* Initialize modules */
	MODULE_INITIALISE_ALL(PIOS_WDG_Clear);

	/* create all modules thread */
	MODULE_TASKCREATE_ALL;

	/* terminate this task */
	PIOS_Thread_Delete(NULL);
}

/**
 * @}
 * @}
 */
