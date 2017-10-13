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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */

#ifndef PIOS_SERVO_H
#define PIOS_SERVO_H

#include <pios_dio.h>

#define PIOS_SERVO_MAX_BANKS 6

/* Used in out_rate.  If 65535, we really mean 65535Hz.  Values close to that
 * are reserved/abused for dshot support.  0 is used for sync pwm per legacy
 */
enum pios_servo_shot_type {
	SHOT_ONESHOT = 0,
	SHOT_DSHOT300 = 65532,
	SHOT_DSHOT600 = 65533,
	SHOT_DSHOT1200 = 65534
};

struct pios_servo_callbacks {
	int (*set_mode)(const uint16_t *out_rate, const int banks,
		const uint16_t *channel_max, const uint16_t *channel_min);

	void (*set)(uint8_t servo, float position);

	void (*update)();
};

extern int PIOS_Servo_GetPins(dio_tag_t *dios, int max_dio);

/* Only applicable to simulation; takes reference to cb */
extern void PIOS_Servo_SetCallbacks(const struct pios_servo_callbacks *cb);

/* Public Functions */
extern int PIOS_Servo_SetMode(const uint16_t *out_rate, const int banks, const uint16_t *channel_max, const uint16_t *channel_min);

#ifndef SIM_POSIX
extern void PIOS_Servo_SetFraction(uint8_t servo, uint16_t fraction,
		uint16_t max_val, uint16_t min_val);
extern void PIOS_Servo_DisableChannel(int channel);
#endif

extern void PIOS_Servo_PrepareForReset();
extern void PIOS_Servo_Set(uint8_t servo, float position);
extern void PIOS_Servo_Update(void);
extern bool PIOS_Servo_IsDshot(uint8_t servo);

#endif /* PIOS_SERVO_H */

/**
  * @}
  * @}
  */
