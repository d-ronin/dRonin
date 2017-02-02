/**
 ******************************************************************************
 *
 * @file       pios_annunc.c   
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Annunciator functions, init, toggle, on & off.
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   PIOS_ANNUNC Annunciator functions
 * @{
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


/* Project Includes */
#include "pios.h"
#include "openpilot.h"
#include "hwsimulation.h"

#if defined(PIOS_INCLUDE_ANNUNC)

/* Private Function Prototypes */

DONT_BUILD_IF(PIOS_ANNUNC_NUM < HWSIMULATION_LEDSTATE_NUMELEM, piosLedNumWrong);
DONT_BUILD_IF(PIOS_LED_ALARM != HWSIMULATION_LEDSTATE_ALARM, idxMismatch1);
DONT_BUILD_IF(PIOS_LED_HEARTBEAT != HWSIMULATION_LEDSTATE_HEARTBEAT, idxMismatch2);

/* Local Variables */
static uint8_t ledState[PIOS_ANNUNC_NUM];

static inline void set_led(uint32_t led, uint8_t stat) {
	PIOS_Assert(led < PIOS_ANNUNC_NUM);

	uint8_t old_state = ledState[led];
	uint8_t new_state = stat ? HWSIMULATION_LEDSTATE_ON : HWSIMULATION_LEDSTATE_OFF;

	if (old_state == new_state)
		return;
	ledState[led] = new_state;

	HwSimulationLedStateSet(ledState);

	char leds[PIOS_ANNUNC_NUM + 1];
	for (int i = 0; i < HWSIMULATION_LEDSTATE_NUMELEM; i++) {
		leds[i] = (ledState[i] == HWSIMULATION_LEDSTATE_ON) ? '*' : '.';
	}
	leds[PIOS_ANNUNC_NUM] = '\0';
	printf("LEDS\t%d\t%d\t%s\n", PIOS_Thread_Systime(),
			PIOS_DELAY_GetRaw(), leds);
}

/**
* Initialises all the LED's
*/
void PIOS_ANNUNC_Init(void)
{
}

/**
* returns whether a given led is on.
*/
bool PIOS_ANNUNC_GetStatus(uint32_t led)
{
	PIOS_Assert(led < PIOS_ANNUNC_NUM);

	return ledState[led] == HWSIMULATION_LEDSTATE_ON;
}

/**
* Turn on LED
* \param[in] LED LED Name (LED1, LED2)
*/
void PIOS_ANNUNC_On(uint32_t led)
{
	set_led(led, 1);
}


/**
* Turn off LED
* \param[in] LED LED Name (LED1, LED2)
*/
void PIOS_ANNUNC_Off(uint32_t led)
{
	set_led(led, 0);
}


/**
* Toggle LED on/off
* \param[in] LED LED Name (LED1, LED2)
*/
void PIOS_ANNUNC_Toggle(uint32_t led)
{
	PIOS_Assert(led < PIOS_ANNUNC_NUM);

	set_led(led, !PIOS_ANNUNC_GetStatus(led));
}

#endif
