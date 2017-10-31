/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_SENSORS Generic sensor interface functions
 * @brief Generic interface for sensors
 * @{
 *
 * @file       pios_sensors.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      Generic interface for sensors
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************/
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

#include "pios_sensors.h"
#include <stddef.h>

//! The list of queue handles
static struct pios_queue *queues[PIOS_SENSOR_LAST];
static uint32_t sample_rates[PIOS_SENSOR_LAST];
static int32_t max_gyro_rate;

#ifdef PIOS_TOLERATE_MISSING_SENSORS
static bool missing_sensors[PIOS_SENSOR_LAST];
#endif

int32_t PIOS_SENSORS_Init()
{
	for (uint32_t i = 0; i < PIOS_SENSOR_LAST; i++) {
		queues[i] = NULL;
		sample_rates[i] = 0;
	}

	return 0;
}

int32_t PIOS_SENSORS_Register(enum pios_sensor_type type, struct pios_queue *queue)
{
	if(queues[type] != NULL)
		return -1;

	queues[type] = queue;

	return 0;
}

bool PIOS_SENSORS_IsRegistered(enum pios_sensor_type type)
{
	if(type >= PIOS_SENSOR_LAST)
		return false;

	if(queues[type] != NULL)
		return true;

	return false;
}

static inline struct pios_queue *PIOS_SENSORS_GetQueue(enum pios_sensor_type type)
{
	if (type >= PIOS_SENSOR_LAST)
		return NULL;

	return queues[type];
}

bool PIOS_SENSORS_GetData(enum pios_sensor_type type, void *buf, int ms_to_wait)
{
	struct pios_queue *q = PIOS_SENSORS_GetQueue(type);

	if (q == NULL) return false;

	return PIOS_Queue_Receive(q, buf, ms_to_wait);
}

void PIOS_SENSORS_SetMaxGyro(int32_t rate)
{
	max_gyro_rate = rate;
}

int32_t PIOS_SENSORS_GetMaxGyro()
{
	return max_gyro_rate;
}

void PIOS_SENSORS_SetSampleRate(enum pios_sensor_type type, uint32_t sample_rate)
{
	if (type >= PIOS_SENSOR_LAST)
		return;

	sample_rates[type] = sample_rate;
}

uint32_t PIOS_SENSORS_GetSampleRate(enum pios_sensor_type type)
{
	if (type >= PIOS_SENSOR_LAST)
		return 0;

	return sample_rates[type];
}

void PIOS_SENSORS_SetMissing(enum pios_sensor_type type)
{
	PIOS_Assert(type < PIOS_SENSOR_LAST);
#ifdef PIOS_TOLERATE_MISSING_SENSORS
	missing_sensors[type] = true;
#endif
}

bool PIOS_SENSORS_GetMissing(enum pios_sensor_type type)
{
	PIOS_Assert(type < PIOS_SENSOR_LAST);

#ifdef PIOS_TOLERATE_MISSING_SENSORS
	return missing_sensors[type];
#else
	return false;
#endif
}
