/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup Revolution OpenPilot Revolution support files
 * @{
 *
 * @file       revolution.c 
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @brief      Start RTOS and the Modules.
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
#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
#include <winsock2.h>
#endif

#include "openpilot.h"
#include "uavobjectsinit.h"
#include "systemmod.h"
#include "pios_thread.h"

/* Prototype of PIOS_Board_Init() function */
extern void PIOS_Board_Init(void);
extern void Stack_Change(void);

/* Local Variables */
#define INIT_TASK_PRIORITY	PIOS_THREAD_PRIO_HIGHEST
#define INIT_TASK_STACK		262144
static struct pios_thread *initTaskHandle;

/* Function Prototypes */
static void initTask(void *parameters);

/**
 * dRonin Main function:
 *
 * Initialize PiOS<BR>
 * Create the "System" task (SystemModInitializein Modules/System/systemmod.c) <BR>
 * Start the RTOS Scheduler<BR>
 * If something goes wrong, blink LED1 and LED2 every 100ms
 *
 */
int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, 1);
	printf("Beginning simulation environment\n");
#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	PIOS_SYS_Args(argc, argv);

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

	initTaskHandle = PIOS_Thread_Create(initTask, "init", INIT_TASK_STACK, NULL, INIT_TASK_PRIORITY);
	PIOS_Assert(initTaskHandle != NULL);

	PIOS_Thread_Sleep(PIOS_THREAD_TIMEOUT_MAX);

	printf("Reached end of main\n");

	return 0;
}

MODULE_INITSYSTEM_DECLS;

/**
 * Initialization task.
 *
 * Runs board and module initialization, then terminates.
 */
void initTask(void *parameters)
{
	printf("Initialization task running\n");
	/* board driver init */
	PIOS_Board_Init();

	/* Initialize modules */
	MODULE_INITIALISE_ALL(PIOS_WDG_Clear);

	printf("Initialization task completed\n");

	/* terminate this task */
	PIOS_Thread_Delete(NULL);
}

/**
 * @}
 * @}
 */
