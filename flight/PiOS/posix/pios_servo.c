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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */

/* Project Includes */
#include "pios.h"

#if defined(PIOS_INCLUDE_SERVO)

/* Private Function Prototypes */

static const struct pios_servo_callbacks *servo_cbs;

void PIOS_Servo_SetCallbacks(const struct pios_servo_callbacks *cb) {
	PIOS_IRQ_Disable();
	servo_cbs = cb;
	PIOS_IRQ_Enable();
}


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
int PIOS_Servo_SetMode(const uint16_t *out_rate, const int banks, const uint16_t *channel_max, const uint16_t *channel_min)
{
	if (servo_cbs && servo_cbs->set_mode) {
		return servo_cbs->set_mode(out_rate, banks, channel_max, channel_min);
	}

	return -200;		/* No impl */
}

/**
* Set servo position
* \param[in] Servo Servo number (0->num_channels-1)
* \param[in] Position Servo position in microseconds
*/
void PIOS_Servo_Set(uint8_t servo, float position)
{
	if (servo_cbs && servo_cbs->set) {
		servo_cbs->set(servo, position);
	}
}

/**
* Update the timer for HPWM/OneShot
*/
void PIOS_Servo_Update(void)
{
	if (servo_cbs && servo_cbs->update) {
		servo_cbs->update();
	}
}

void PIOS_Servo_PrepareForReset() {
}

bool PIOS_Servo_IsDshot(uint8_t servo) {
	(void) servo;

	return false;
}

#endif
