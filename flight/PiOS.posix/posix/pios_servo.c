/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_SERVO RC Servo Functions
 * @brief Code to do set RC servo output
 * @{
 *
 * @file       pios_servo.c  
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      RC Servo routines (STM32 dependent)
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

/* Project Includes */
#include "pios.h"

#if defined(PIOS_INCLUDE_SERVO)

/* Private Function Prototypes */

/* Local Variables */
static volatile uint16_t servo_position[PIOS_SERVO_NUM_TIMERS];

/**
* Initialise Servos
*/
void PIOS_Servo_Init(void)
{
}

/**
 * @brief PIOS_Servo_SetMode Sets the PWM output frequency and resolution.
 * An output rate of 0 indicates Synchronous updates (e.g. SyncPWM/OneShot), otherwise
 * normal PWM at the specified output rate.
 * @param out_rate array of output rate in Hz, banks elements
 * @param banks maximum number of banks
 * @param channel_max array of max pulse lengths, number of channels elements
 */
void PIOS_Servo_SetMode(const uint16_t *out_rate, const int banks, const uint16_t *channel_max)
{
}

/**
* Set servo position
* \param[in] Servo Servo number (0->num_channels-1)
* \param[in] Position Servo position in microseconds
*/
void PIOS_Servo_Set(uint8_t servo, float position)
{
#ifndef PIOS_ENABLE_DEBUG_PINS
	/* Make sure servo exists */
	if (servo < PIOS_SERVO_NUM_OUTPUTS) {
		/* Update the position */
		servo_position[servo] = position;

	}
#endif // PIOS_ENABLE_DEBUG_PINS
}

/**
* Update the timer for HPWM/OneShot
*/
void PIOS_Servo_Update(void)
{
}

#endif
