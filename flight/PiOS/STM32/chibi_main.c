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

#include "pios_config.h"
#include "uavobjectsinit.h"
#include "systemmod.h"
#include "pios_thread.h"

/* Local Variables */
#define INIT_TASK_PRIORITY	PIOS_THREAD_PRIO_HIGHEST
#define INIT_TASK_STACK		1024

/* Function Prototypes */
extern void PIOS_Board_Init(void);
extern void Stack_Change(void);
static void initTask(void *parameters);

static struct pios_thread *initTaskHandle;

void ledinit()
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_5 | GPIO_Pin_4; // we want to configure all LED GPIO pins
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT; 		// we want the pins to be an output
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; 	// this sets the GPIO modules clock speed
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP; 	// this sets the pin type to push / pull (as opposed to open drain)
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP; 	// this sets the pullup / pulldown resistors to be inactive
	GPIO_Init(GPIOB, &GPIO_InitStruct); 		

	GPIO_SetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_5 | GPIO_Pin_4);
}

const int cyc = 50;
int cnt = 0;
void system_tick_led()
{
		if (cnt < cyc)
			GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_5 | GPIO_Pin_4);
		else
			GPIO_SetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_5 | GPIO_Pin_4);
		cnt++;
		if(cnt > (cyc*2)) cnt = 0;
}

void ledcrap()
{
	ledinit();

	const int cycles = 5000000;
	int c = 0;
	while(1)
	{
		if (c < cycles)
			GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_5 | GPIO_Pin_4);
		else
			GPIO_SetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_5 | GPIO_Pin_4);
		c++;
		if(c > (cycles*2)) c = 0;
	}
}

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
	
	/* For Sparky2 we use an RTOS task to bring up the system so we can */
	/* always rely on an RTOS primitive */	
	ledinit();
	//while(1);

	initTaskHandle = PIOS_Thread_Create(initTask, "init", INIT_TASK_STACK, NULL, INIT_TASK_PRIORITY);
	PIOS_Assert(initTaskHandle != NULL);

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

/**
 * Initialisation task.
 *
 * Runs board and module initialisation, then terminates.
 */
void initTask(void *parameters)
{
	// ledcrap(); // Blink Revo LED if we get here.

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
 */
