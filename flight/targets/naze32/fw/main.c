/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup Naze32 family support files
 * @{
 *
 * @file       main.c 
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
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
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */


/* OpenPilot Includes */
#include "openpilot.h"
#include "uavobjectsinit.h"
#include "systemmod.h"

#if defined(PIOS_INCLUDE_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif /* defined(PIOS_INCLUDE_FREERTOS) */

/* Prototype of PIOS_Board_Init() function */
extern void PIOS_Board_Init(void);

/**
* dRonin Main function:
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

	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();

	/* Architecture dependant Hardware and
	 * core subsystem initialisation
	 * (see pios_board.c for your arch)
	 * */
	PIOS_Board_Init();
	PIOS_WDG_Clear();

	/* Initialize modules */
	MODULE_INITIALISE_ALL(PIOS_WDG_Clear);

	/* swap the stack to use the IRQ stack */
	//Stack_Change();
#ifdef PIOS_INCLUDE_FREERTOS
	/* Start the FreeRTOS scheduler */
	vTaskStartScheduler();

	/* If all is well we will never reach here as the scheduler will now be running. */
	/* Do some PIOS_LED_HEARTBEAT to user that something bad just happened */
	PIOS_LED_Off(PIOS_LED_HEARTBEAT); \
	for(;;) { \
		PIOS_LED_Toggle(PIOS_LED_HEARTBEAT); \
		PIOS_DELAY_WaitmS(100); \
	};
#else
	#error Requires FreeRTOS
#endif // PIOS_INCLUDE_FREERTOS

	return 0;
}

/**
 * @}
 * @}
 */

