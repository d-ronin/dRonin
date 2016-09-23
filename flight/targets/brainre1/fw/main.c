/**
******************************************************************************
* @addtogroup TauLabsTargets Tau Labs Targets
* @{
* @addtogroup FlyingF4 FlyingF4 support files
* @{
*
* @file       main.c
* @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
*             AJ Christensen <aj@junglistheavy.industries>
* @brief      Start ChiBiOS/RT and the Modules.
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
#include "openpilot.h"
#include "uavobjectsinit.h"
#include "systemmod.h"
#include "pios_thread.h"
#include "hwbrainre1.h"

#if defined(PIOS_INCLUDE_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif /* defined(PIOS_INCLUDE_FREERTOS) */

/* Prototype of PIOS_Board_Init() function */
extern void PIOS_Board_Init(void);
extern void Stack_Change(void);
void ledUpdatePeridodicCb(UAVObjEvent * ev, void *ctx, void *obj, int len);


/* Local Variables */
#define INIT_TASK_PRIORITY  PIOS_THREAD_PRIO_HIGHEST
#define INIT_TASK_STACK   1024
static struct pios_thread *initTaskHandle;


/* Function Prototypes */
static void initTask(void *parameters);

/**
 * Tau Labs Main function:
 *
 * Initialize PiOS<BR>
 * Create the "System" task (SystemModInitializein Modules/System/systemmod.c) <BR>
 * Start FreeRTOS Scheduler (vTaskStartScheduler)<BR>
 * If something goes wrong, blink LED1 and LED2 every 100ms
 *
 */
int main()
{
	/* NOTE: Do NOT modify the following start-up sequence */
	/* Any new initialization functions should be added in OpenPilotInit() */
	PIOS_heap_initialize_blocks();

#if defined(PIOS_INCLUDE_CHIBIOS)
	halInit();
	chSysInit();

	boardInit();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();

	/* For Revolution we use a FreeRTOS task to bring up the system so we can */
	/* always rely on FreeRTOS primitive */
	initTaskHandle = PIOS_Thread_Create(initTask, "init", INIT_TASK_STACK, NULL, INIT_TASK_PRIORITY);
	PIOS_Assert(initTaskHandle != NULL);

#if defined(PIOS_INCLUDE_FREERTOS)
	/* Start the FreeRTOS scheduler */
	vTaskStartScheduler();

	/* If all is well we will never reach here as the scheduler will now be running. */
	/* Do some PIOS_LED_HEARTBEAT to user that something bad just happened */
	PIOS_LED_Off(PIOS_LED_HEARTBEAT); \
	for(;;) { \
		PIOS_LED_Toggle(PIOS_LED_HEARTBEAT); \
		PIOS_DELAY_WaitmS(100); \
	};
#elif defined(PIOS_INCLUDE_CHIBIOS)
	PIOS_Thread_Sleep(PIOS_THREAD_TIMEOUT_MAX);
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	return 0;
}
/**
 * Initialization task.
 *
 * Runs board and module initialization, then terminates.
 */
void initTask(void *parameters)
{
	/* board driver init */
	PIOS_Board_Init();

	/* Initialize modules */
	MODULE_INITIALISE_ALL(PIOS_WDG_Clear);

	/* Schedule a periodic callback to update the LEDs */
	UAVObjEvent ev = {
		.obj = HwBrainRE1Handle(),
		.instId = 0,
		.event = 0,
	};
	EventPeriodicCallbackCreate(&ev, ledUpdatePeridodicCb, 31);

	/* terminate this task */
	PIOS_Thread_Delete(NULL);
}

/**
 * @}
 * @}
 */
