/**
 ******************************************************************************
 *
 * @file       pios_led.c   
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      LED functions, init, toggle, on & off.
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   PIOS_LED LED Functions
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
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


/* Project Includes */
#include "pios.h"
#include "openpilot.h"
#include "hwsimulation.h"

#if defined(PIOS_INCLUDE_LED)

/* Private Function Prototypes */

DONT_BUILD_IF(PIOS_LED_NUM != HWSIMULATION_LEDSTATE_NUMELEM, piosLedNumWrong);

/* Local Variables */
static uint8_t ledState[HWSIMULATION_LEDSTATE_NUMELEM];

static int map_pios_led_to_uavo(const uint32_t led)
{
	switch (led) {
	case PIOS_LED_ALARM:
		return HWSIMULATION_LEDSTATE_ALARM;
	case PIOS_LED_HEARTBEAT:
		return HWSIMULATION_LEDSTATE_HEARTBEAT;
	default:
		PIOS_Assert(0);
	}
}

static inline void PIOS_SetLED(uint32_t led, uint8_t stat) {
	PIOS_Assert(led < PIOS_LED_NUM);

	uint8_t old_state = ledState[map_pios_led_to_uavo(led)];
	ledState[map_pios_led_to_uavo(led)] = stat ? HWSIMULATION_LEDSTATE_ON : HWSIMULATION_LEDSTATE_OFF;

	HwSimulationLedStateSet(ledState);

	if (old_state == ledState[map_pios_led_to_uavo(led)])
		return;

	char leds[HWSIMULATION_LEDSTATE_NUMELEM + 1];
	for (int i = 0; i < HWSIMULATION_LEDSTATE_NUMELEM; i++) {
		leds[i] = (ledState[i] == HWSIMULATION_LEDSTATE_ON) ? '*' : '.';
	}
	leds[PIOS_LED_NUM] = '\0';
	printf("LED State: [%s]\n", leds);
}

/**
* Initialises all the LED's
*/
void PIOS_LED_Init(void)
{
	PIOS_Assert(0);
	HwSimulationLedStateGet(ledState);
}


/**
* Turn on LED
* \param[in] LED LED Name (LED1, LED2)
*/
void PIOS_LED_On(uint32_t led)
{
	PIOS_SetLED(led, 1);
}


/**
* Turn off LED
* \param[in] LED LED Name (LED1, LED2)
*/
void PIOS_LED_Off(uint32_t led)
{
	PIOS_SetLED(led, 0);
}


/**
* Toggle LED on/off
* \param[in] LED LED Name (LED1, LED2)
*/
void PIOS_LED_Toggle(uint32_t led)
{
	PIOS_Assert(led < PIOS_LED_NUM);

	PIOS_SetLED(led, ((ledState[map_pios_led_to_uavo(led)] == HWSIMULATION_LEDSTATE_ON) ? 0 : 1));
}

#endif
