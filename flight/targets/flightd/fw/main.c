/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup Sim POSIX Simulator
 * @{
 *
 * @file       simulation/fw/main.c
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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */

/* OpenPilot Includes */
#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
#include <winsock2.h>
#endif

#include "openpilot.h"
#include "uavobjectsinit.h"
#include "systemmod.h"
#include "pios_thread.h"
#include "pios_hal.h"

/* Prototype of PIOS_Board_Init() function */
extern void PIOS_Board_Init(void);
extern void Stack_Change(void);

/* Function Prototypes */
static void initTask();

static int g_argc;
static char **g_argv;

int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, 1);
	printf("Beginning simulation environment\n");
#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	g_argc = argc;
	g_argv = argv;

	PIOS_heap_initialize_blocks();

	initTask();

	return 0;
}

MODULE_INITSYSTEM_DECLS;

void system_task();

/**
 * Initialization task.
 *
 * Runs board and module initialization, then terminates.
 */
void initTask()
{
	printf("Initialization task running\n");

	PIOS_SYS_Init();

	UAVObjInitialize();

	/* Initialize the task monitor library */
	TaskMonitorInitialize();

	PIOS_HAL_InitUAVTalkReceiver();

	PIOS_SYS_Args(g_argc, g_argv);

	/* board driver init */
	PIOS_Board_Init();

	/* Initialize modules */
	MODULE_INITIALISE_ALL(PIOS_WDG_Clear);

	/* create all modules thread */
	MODULE_TASKCREATE_ALL;

	printf("Initialization task completed-- invoking system task\n");

	system_task();

	printf("System task exited?  Shouldn't happen.\n");
}

/**
 * @}
 * @}
 */
