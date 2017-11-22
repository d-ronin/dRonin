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

//! The list of queue handles / callbacks
static struct PIOS_Sensor {
	PIOS_SENSOR_Callback_t getdata_cb;
	void *getdata_ctx;

	uint16_t sample_rate;
	uint16_t missing : 1;
} sensors[PIOS_SENSOR_NUM];

static int32_t max_gyro_rate;

int32_t PIOS_SENSORS_Init()
{
	/* sensors array is on BSS and pre-zero'd */

	return 0;
}

static bool PIOS_SENSORS_QueueCallback(void *ctx, void *buf,
		int ms_to_wait, int *next_call)
{
	struct pios_queue *q = ctx;

	*next_call = 0;		/* May immediately have data on next call */

	return PIOS_Queue_Receive(q, buf, ms_to_wait);
}

int32_t PIOS_SENSORS_RegisterCallback(enum pios_sensor_type type,
		PIOS_SENSOR_Callback_t callback, void *ctx)
{
	PIOS_Assert(type < PIOS_SENSOR_NUM);

	struct PIOS_Sensor *sensor = &sensors[type];

	sensor->getdata_ctx = ctx;
	sensor->getdata_cb = callback;
	sensor->missing = 0;

	return 0;
}

int32_t PIOS_SENSORS_Register(enum pios_sensor_type type, struct pios_queue *queue)
{
	return PIOS_SENSORS_RegisterCallback(type,
			PIOS_SENSORS_QueueCallback, queue);
}

bool PIOS_SENSORS_IsRegistered(enum pios_sensor_type type)
{
	if (type >= PIOS_SENSOR_NUM) {
		return false;
	}

	struct PIOS_Sensor *sensor = &sensors[type];

	if (sensor->missing) {
		return false;
	}

	return sensor->getdata_cb != NULL;
}

bool PIOS_SENSORS_GetData(enum pios_sensor_type type, void *buf, int ms_to_wait)
{
	if (type >= PIOS_SENSOR_NUM) {
		return false;
	}

	struct PIOS_Sensor *sensor = &sensors[type];

	if (!sensor->getdata_cb) {
		return false;
	}

	int next_time;

	bool ret = sensor->getdata_cb(sensor->getdata_ctx, buf, ms_to_wait,
			&next_time);

	/* TODO: keep track of next time it *could* have data to use in
	 * scheduling.
	 */

	return ret;
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
	PIOS_Assert(type < PIOS_SENSOR_NUM);

	struct PIOS_Sensor *sensor = &sensors[type];

	sensor->sample_rate = sample_rate;
}

uint32_t PIOS_SENSORS_GetSampleRate(enum pios_sensor_type type)
{
	if (type >= PIOS_SENSOR_NUM)
		return 0;

	struct PIOS_Sensor *sensor = &sensors[type];

	return sensor->sample_rate;
}

void PIOS_SENSORS_SetMissing(enum pios_sensor_type type)
{
	PIOS_Assert(type < PIOS_SENSOR_NUM);

	struct PIOS_Sensor *sensor = &sensors[type];

	sensor->missing = true;
}

bool PIOS_SENSORS_GetMissing(enum pios_sensor_type type)
{
	PIOS_Assert(type < PIOS_SENSOR_NUM);

	struct PIOS_Sensor *sensor = &sensors[type];

	return sensor->missing;
}
