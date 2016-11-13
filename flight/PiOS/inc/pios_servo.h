/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_SERVO RC Servo Functions
 * @{
 *
 * @file       pios_servo.h  
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
 * @brief      RC Servo functions header.
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

#ifndef PIOS_SERVO_H
#define PIOS_SERVO_H

#define PIOS_SERVO_MAX_BANKS 6

struct pios_servo_callbacks {
	void (*set_mode)(const uint16_t *out_rate, const int banks,
		const uint16_t *channel_max, const uint16_t *channel_min);

	void (*set)(uint8_t servo, float position);

	void (*update)();
};

/* Only applicable to simulation; takes reference to cb */
extern void PIOS_Servo_SetCallbacks(const struct pios_servo_callbacks *cb);

/* Public Functions */
extern void PIOS_Servo_SetMode(const uint16_t *out_rate, const int banks, const uint16_t *channel_max, const uint16_t *channel_min);

#ifndef SIM_POSIX
extern void PIOS_Servo_SetFraction(uint8_t servo, uint16_t fraction,
		uint16_t max_val, uint16_t min_val);
#endif

extern void PIOS_Servo_PrepareForReset();
extern void PIOS_Servo_Set(uint8_t servo, float position);
extern void PIOS_Servo_Update(void);

#endif /* PIOS_SERVO_H */

/**
  * @}
  * @}
  */
