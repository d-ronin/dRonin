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
	PIOS_SENSOR_LAST
};

//! Initialize the PIOS_SENSORS interface
int32_t PIOS_SENSORS_Init();

//! Register a sensor with the PIOS_SENSORS interface
int32_t PIOS_SENSORS_Register(enum pios_sensor_type type, struct pios_queue *queue);

//! Checks if a sensor type is registered with the PIOS_SENSORS interface
bool PIOS_SENSORS_IsRegistered(enum pios_sensor_type type);

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

/*
	^^^ Old sensor stuff.
--------------------------------------------
	vvv New sensor stuff.
*/

typedef int (*pios_sensors_cue_call)(void *dev);
typedef int (*pios_sensors_callback)(void *dev, void *output);

typedef struct pios_sensors_info* pios_sensor_t;

/* Something bad happened. */
#define PIOS_SENSORS_DATA_ERROR						-1
/* There's no data available on the polling cycle. */
#define PIOS_SENSORS_DATA_NONE						0
/* Sensor is busy doing previously requested work. */
#define PIOS_SENSORS_DATA_BUSY						1
/* Sensor is waiting for a cue/action to finish. */
#define PIOS_SENSORS_DATA_WAITING					2
/* Sensor has returned data. */
#define PIOS_SENSORS_DATA_AVAILABLE					3

/* Nothing special. */
#define PIOS_SENSORS_FLAG_NONE						0
/* Create a semaphore the sensor can trigger and used with _WaitOn. */
#define PIOS_SENSORS_FLAG_SEMAPHORE					0x0010
/* Sensor is polled, runs on a specified schedule. */
#define PIOS_SENSORS_FLAG_SCHEDULED					0x0020
/* For internal use, remapping old queues to the new sensor code. */
#define PIOS_SENSORS_FLAG_QUEUE						0x0040
/* If this is set, the sensor is considered a goner. */
#define PIOS_SENSORS_FLAG_MISSING					0x0080
/* Marks a sensor as an early registration. To deal with old sensors updating
   settings before the sensor is actually registered. */
#define PIOS_SENSORS_FLAG_EARLY_REG						0x0100

void PIOS_Sensors_Initialize();
pios_sensor_t PIOS_Sensors_Register(enum pios_sensor_type sensor_type, void *device, const pios_sensors_callback callback, uint16_t flags);

void PIOS_Sensors_SetRange(pios_sensor_t sensor, float range);
void PIOS_Sensors_SetUpdateRate(pios_sensor_t sensor, uint32_t samplerate);
float PIOS_Sensors_GetRange(pios_sensor_t sensor);
uint32_t PIOS_Sensors_GetUpdateRate(pios_sensor_t sensor);
void PIOS_Sensors_SetMissing(enum pios_sensor_type sensor_type);
bool PIOS_Sensors_IsMissing(enum pios_sensor_type sensor_type);
uint16_t PIOS_Sensors_GetFlags(pios_sensor_t sensor);

void PIOS_Sensors_SetSchedule(pios_sensor_t sensor, uint32_t microseconds);
void PIOS_Sensors_RaiseSignal(pios_sensor_t sensor);
void PIOS_Sensors_RaiseSignal_FromISR(pios_sensor_t sensor);
bool PIOS_Sensors_WaitOn(pios_sensor_t sensor, int timeout);
bool PIOS_Sensors_Available(enum pios_sensor_type sensor_type);

uint8_t PIOS_Sensors_GetLRU();
int PIOS_Sensors_GetData(pios_sensor_t sensor, void *output);

pios_sensor_t PIOS_Sensors_GetSensor(enum pios_sensor_type sensor_type);

#endif /* PIOS_SENSOR_H */
