/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_SENSORS Generic sensor interface functions
 * @brief Generic interface for sensors
 * @{
 *
 * @file       pios_sensors.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
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

#ifndef PIOS_SENSOR_H
#define PIOS_SENSOR_H

#include "pios.h"
#include "stdint.h"
#include "pios_queue.h"

//! Pios sensor structure for generic gyro data
struct pios_sensor_gyro_data {
	float x;
	float y; 
	float z;
	float temperature;
};

//! Pios sensor structure for generic accel data
struct pios_sensor_accel_data {
	float x;
	float y; 
	float z;
	float temperature;
};

//! Pios sensor structure for generic mag data
struct pios_sensor_mag_data {
	float x;
	float y; 
	float z;
};

//! Pios sensor structure for generic mag data
struct pios_sensor_optical_flow_data {
	float x_dot;
	float y_dot;
	float z_dot;

	uint8_t quality;
};

//! Pios sensor structure for generic rangefinder data
struct pios_sensor_rangefinder_data {
	float range;
	float velocity;
	bool range_status;
	bool velocity_status;
};

//! Pios sensor structure for generic baro data
struct pios_sensor_baro_data {
	float temperature;
	float pressure;
	float altitude;
};

//! The types of sensors this module supports
enum pios_sensor_type
{
	PIOS_SENSOR_ACCEL,
	PIOS_SENSOR_GYRO,
	PIOS_SENSOR_MAG,
	PIOS_SENSOR_BARO,
	PIOS_SENSOR_OPTICAL_FLOW,
	PIOS_SENSOR_RANGEFINDER,
	PIOS_SENSOR_NUM
};

//! Function that calls into sensor to get data.
typedef bool (*PIOS_SENSOR_Callback_t)(void *ctx, void *output,
		int ms_to_wait, int *next_call);

//! Initialize the PIOS_SENSORS interface
int32_t PIOS_SENSORS_Init();

//! Register a queue-based sensor with the PIOS_SENSORS interface
int32_t PIOS_SENSORS_Register(enum pios_sensor_type type, struct pios_queue *queue);

//! Register a callback-based sensor with the PIOS_SENSORS interface
int32_t PIOS_SENSORS_RegisterCallback(enum pios_sensor_type type,
		PIOS_SENSOR_Callback_t callback, void *ctx);

//! Checks if a sensor type is registered with the PIOS_SENSORS interface
bool PIOS_SENSORS_IsRegistered(enum pios_sensor_type type);

//! Get the data for a sensor type
bool PIOS_SENSORS_GetData(enum pios_sensor_type type, void *buf, int ms_to_wait);

//! Set the maximum gyro rate in deg/s
void PIOS_SENSORS_SetMaxGyro(int32_t rate);

//! Get the maximum gyro rate in deg/s
int32_t PIOS_SENSORS_GetMaxGyro();

//! Set the sample rate of a sensor (Hz)
void PIOS_SENSORS_SetSampleRate(enum pios_sensor_type type, uint32_t sample_rate);

//! Get the sample rate of a sensor (Hz)
uint32_t PIOS_SENSORS_GetSampleRate(enum pios_sensor_type type);

//! Assert that an optional (non-accel/gyro), but expected sensor is missing
void PIOS_SENSORS_SetMissing(enum pios_sensor_type type);

//! Determine if an optional but expected sensor is missing.
bool PIOS_SENSORS_GetMissing(enum pios_sensor_type type);

#endif /* PIOS_SENSOR_H */
