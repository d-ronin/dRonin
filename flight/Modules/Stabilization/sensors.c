/**
 ******************************************************************************
 * @addtogroup Modules Modules
 * @{
 * @addtogroup StabilizationModule Stabilization Module
 * @{
 *
 * @file       sensors.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2016
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2017
 * @brief      Acquire sensor data from sensors registered with @ref PIOS_Sensors
 *
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

#include "openpilot.h"
#include "pios.h"
#include "physical_constants.h"
#include "pios_thread.h"
#include "pios_queue.h"
#include "misc_math.h"
#include "lpfilter.h"
#include "sensors.h"

#if defined(PIOS_INCLUDE_PX4FLOW)
#include "pios_px4flow_priv.h"
extern pios_i2c_t external_i2c_adapter_id;
#endif /* PIOS_INCLUDE_PX4FLOW */

// UAVOs
#include "accels.h"
#include "attitudeactual.h"
#include "attitudesettings.h"
#include "baroaltitude.h"
#include "gyros.h"
#include "gyrosbias.h"
#include "homelocation.h"
#include "opticalflowsettings.h"
#include "opticalflow.h"
#include "sensorsettings.h"
#include "rangefinder.h"
#include "inssettings.h"
#include "magnetometer.h"
#include "magbias.h"
#include "coordinate_conversions.h"

// Private constants
#define MAX_SENSOR_PERIOD 6	// allow sensor data as slow as 166Hz
#define REQUIRED_GOOD_CYCLES 50
#define MAX_TIME_BETWEEN_VALID_BARO_DATAS_US (100*1000)
#define MAX_TIME_BETWEEN_VALID_MAG_DATAS_US (300*1000)

// Private types
enum mag_calibration_algo {
	MAG_CALIBRATION_PRELEMARI,
	MAG_CALIBRATION_NORMALIZE_LENGTH
};

// Private functions
static void update_accels(struct pios_sensor_accel_data *accel);
static void update_gyros(struct pios_sensor_gyro_data *gyro);
static void update_mags(struct pios_sensor_mag_data *mag);
static void update_baro(struct pios_sensor_baro_data *baro);

#if defined (PIOS_INCLUDE_OPTICALFLOW)
static void update_optical_flow(struct pios_sensor_optical_flow_data *optical_flow);
#endif /* PIOS_INCLUDE_OPTICALFLOW */

#if defined (PIOS_INCLUDE_RANGEFINDER)
static void update_rangefinder(struct pios_sensor_rangefinder_data *rangefinder);
#endif /* PIOS_INCLUDE_RANGEFINDER */

#ifdef PIOS_INCLUDE_SIMSENSORS
extern int32_t simsensors_init(void);
#endif

static void mag_calibration_prelemari(MagnetometerData *mag);
static void mag_calibration_fix_length(MagnetometerData *mag);

static void updateTemperatureComp(float temperature, float *temp_bias);
static void sensors_settings_update();

// Private variables
static INSSettingsData insSettings;
static AccelsData accelsData;

static volatile bool settings_updated = true;

// These values are initialized by settings but can be updated by the attitude algorithm
static bool bias_correct_gyro = true;

#ifdef PIOS_TOLERATE_MISSING_SENSORS
SystemAlarmsAlarmOptions missing_sensor_severity = SYSTEMALARMS_ALARM_ERROR;
#endif

static float mag_bias[3] = {0,0,0};
static float mag_scale[3] = {0,0,0};
static float accel_bias[3] = {0,0,0};
static float accel_scale[3] = {0,0,0};
static float gyro_scale[3] = {0,0,0};
static float gyro_coeff_x[4] = {0,0,0,0};
static float gyro_coeff_y[4] = {0,0,0,0};
static float gyro_coeff_z[4] = {0,0,0,0};
static float gyro_temp_bias[3] = {0,0,0};
static float z_accel_offset = 0;
static float Rsb[3][3] = {{0}}; //! Rotation matrix that transforms from the body frame to the sensor board frame
static int8_t rotate = 0;

//! Select the algorithm to try and null out the magnetometer bias error
static enum mag_calibration_algo mag_calibration_algo = MAG_CALIBRATION_PRELEMARI;

static lpfilter_state_t gyro_filter;
static lpfilter_state_t accel_filter;

/**
 * API for sensor fusion algorithms:
 * Configure(struct pios_queue *gyro, struct pios_queue *accel, struct pios_queue *mag, struct pios_queue *baro)
 *   Stores all the queues the algorithm will pull data from
 * FinalizeSensors() -- before saving the sensors modifies them based on internal state (gyro bias)
 * Update() -- queries queues and updates the attitude estiamte
 */

/**
 * Initialise the module.  Called before the start function
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t sensors_init(void)
{
	if (GyrosInitialize() == -1 \
		|| GyrosBiasInitialize() == -1 \
		|| AccelsInitialize() == -1 \
		|| BaroAltitudeInitialize() == -1 \
		|| MagnetometerInitialize() == -1 \
		|| MagBiasInitialize() == -1 \
		|| AttitudeSettingsInitialize() == -1 \
		|| SensorSettingsInitialize() == -1 \
		|| INSSettingsInitialize() == -1) {

		return -1;
	}

#if defined (PIOS_INCLUDE_OPTICALFLOW)
	if (OpticalFlowSettingsInitialize() == -1){
		return -1;
	}
	OpticalFlowSettingsData opticalFlowSettings;
	OpticalFlowSettingsGet(&opticalFlowSettings);
	switch (opticalFlowSettings.SensorType ){
		case OPTICALFLOWSETTINGS_SENSORTYPE_PX4FLOW:
#if defined(PIOS_INCLUDE_PX4FLOW)
		{
			if (external_i2c_adapter_id) {
				struct pios_px4flow_cfg pios_px4flow_cfg;
				pios_px4flow_cfg.rotation.roll_D100 = opticalFlowSettings.SensorRotation[OPTICALFLOWSETTINGS_SENSORROTATION_ROLL];
				pios_px4flow_cfg.rotation.pitch_D100 = opticalFlowSettings.SensorRotation[OPTICALFLOWSETTINGS_SENSORROTATION_PITCH];
				pios_px4flow_cfg.rotation.yaw_D100 = opticalFlowSettings.SensorRotation[OPTICALFLOWSETTINGS_SENSORROTATION_YAW];
				if (PIOS_PX4Flow_Init(&pios_px4flow_cfg, external_i2c_adapter_id) != 0) {
					// set alarm
				}
			}
		}
#endif /* PIOS_INCLUDE_PX4FLOW */
		break;
	}

	if (PIOS_SENSORS_IsRegistered(PIOS_SENSOR_OPTICAL_FLOW)) {
		if (OpticalFlowInitialize() == -1) {
			return -1;
		}
	}
#endif /* PIOS_INCLUDE_OPTICALFLOW */

#if defined (PIOS_INCLUDE_RANGEFINDER)
	if (PIOS_SENSORS_IsRegistered(PIOS_SENSOR_RANGEFINDER)) {
		if (RangefinderInitialize() == -1) {
			return -1;
		}
	}
#endif /* PIOS_INCLUDE_RANGEFINDER */

	rotate = 0;

	AttitudeSettingsConnectCallbackCtx(UAVObjCbSetFlag, &settings_updated);
	SensorSettingsConnectCallbackCtx(UAVObjCbSetFlag, &settings_updated);
	INSSettingsConnectCallbackCtx(UAVObjCbSetFlag, &settings_updated);

#ifdef PIOS_INCLUDE_SIMSENSORS
	simsensors_init();
#endif

	return 0;
}

/**
 * This polls the gyros and pumps that data to other users.
 */
bool sensors_step()
{
	static uint32_t good_runs = 0;
	static uint32_t last_baro_update_time;
	static uint32_t last_mag_update_time;

	bool ret = false;	/* Are gyros OK this time? */

	if (settings_updated) {
		sensors_settings_update();
	}

	struct pios_sensor_gyro_data gyros;
	struct pios_sensor_accel_data accels;
	struct pios_sensor_mag_data mags;
	struct pios_sensor_baro_data baro;

	uint32_t timeval = PIOS_DELAY_GetRaw();

	// First do everything NON-gyro/accel, so as to minimize gyro->stab
	// latency.
	bool good_run = true;

	SystemAlarmsAlarmOptions fallback_severity = SYSTEMALARMS_ALARM_OK;

	if (PIOS_SENSORS_GetData(PIOS_SENSOR_MAG, &mags, 0) != false) {
		// we can use the timeval because it contains the current time stamp (PIOS_DELAY_GetRaw())
		last_mag_update_time = timeval;
		update_mags(&mags);
#ifdef PIOS_TOLERATE_MISSING_SENSORS
	} else if (PIOS_SENSORS_GetMissing(PIOS_SENSOR_MAG)) {
		fallback_severity = missing_sensor_severity;
#endif
	} else if (PIOS_SENSORS_IsRegistered(PIOS_SENSOR_MAG)) {
		uint32_t dT_mag_datas = PIOS_DELAY_DiffuS(last_mag_update_time);
		// if the last valid sensor datas older than 300 ms report an error
		if (dT_mag_datas > MAX_TIME_BETWEEN_VALID_MAG_DATAS_US) {
			good_run = false;
			fallback_severity = SYSTEMALARMS_ALARM_ERROR;
		}
	}

	if (PIOS_SENSORS_GetData(PIOS_SENSOR_BARO, &baro, 0) != false) {
		// we can use the timeval because it contains the current time stamp (PIOS_DELAY_GetRaw())
		last_baro_update_time = timeval;
		update_baro(&baro);
		AlarmsClear(SYSTEMALARMS_ALARM_TEMPBARO);
#ifdef PIOS_TOLERATE_MISSING_SENSORS
	} else if (PIOS_SENSORS_GetMissing(PIOS_SENSOR_BARO)) {
		fallback_severity = missing_sensor_severity;
#endif
	} else if (PIOS_SENSORS_IsRegistered(PIOS_SENSOR_BARO)) {
		// Check that we got valid sensor datas
		uint32_t dT_baro_datas = PIOS_DELAY_DiffuS(last_baro_update_time);
		// if the last valid sensor datas older than 100 ms report an error
		if (dT_baro_datas > MAX_TIME_BETWEEN_VALID_BARO_DATAS_US) {
			good_run = false;
			AlarmsSet(SYSTEMALARMS_ALARM_TEMPBARO, SYSTEMALARMS_ALARM_ERROR);
		}
	}

#if defined(PIOS_INCLUDE_OPTICALFLOW)
	struct pios_sensor_optical_flow_data optical_flow;
	if (PIOS_SENSORS_GetData(PIOS_SENSOR_OPTICAL_FLOW, &optical_flow, 0) != false) {
		update_optical_flow(&optical_flow);
	}
#endif /* PIOS_INCLUDE_OPTICALFLOW */

#if defined(PIOS_INCLUDE_RANGEFINDER)
	struct pios_sensor_rangefinder_data rangefinder;
	if (PIOS_SENSORS_GetData(PIOS_SENSOR_RANGEFINDER, &rangefinder, 0) != false) {
		update_rangefinder(&rangefinder);
	}
#endif /* PIOS_INCLUDE_RANGEFINDER */

	//Block on gyro data but nothing else
	if (PIOS_SENSORS_GetData(PIOS_SENSOR_GYRO, &gyros, MAX_SENSOR_PERIOD) == false) {
		good_run = false;
	} else {
		ret = true;
	}

	if (PIOS_SENSORS_GetData(PIOS_SENSOR_ACCEL, &accels, 0) == false) {
		// If no new accels data is ready, reuse the latest sample
		AccelsSet(&accelsData);
	} else {
		update_accels(&accels);
	}

	// Update gyros after the accels since the rest of the code expects
	// the accels to be available first
	update_gyros(&gyros);

	// Check total time to get the sensors wasn't over the limit
	uint32_t dT_us = PIOS_DELAY_DiffuS(timeval);

#ifdef FLIGHT_POSIX
	if (PIOS_Thread_FakeClock_IsActive()) {
		dT_us = 0;
	}
#endif

	if (dT_us > (MAX_SENSOR_PERIOD * 1000)) {
		good_run = false;
	}

	if (!good_run || (good_runs < REQUIRED_GOOD_CYCLES)) {
		AlarmsSet(SYSTEMALARMS_ALARM_SENSORS, SYSTEMALARMS_ALARM_CRITICAL);

		if (good_run) {
			good_runs++;
		} else {
			good_runs = 0;
		}
	} else {
		AlarmsSet(SYSTEMALARMS_ALARM_SENSORS, fallback_severity);
	}

	return ret;
}

/**
 * @brief Apply calibration and rotation to the raw accel data
 * @param[in] accels The raw accel data
 */
static void update_accels(struct pios_sensor_accel_data *accels)
{
	// Average and scale the accels before rotation
	float accels_out[3] = {
	    accels->x * accel_scale[0] - accel_bias[0],
	    accels->y * accel_scale[1] - accel_bias[1],
	    accels->z * accel_scale[2] - accel_bias[2]
	};

	lpfilter_run(accel_filter, accels_out);

	if (rotate) {
		float accel_rotated[3];
		rot_mult(Rsb, accels_out, accel_rotated, true);
		accelsData.x = accel_rotated[0];
		accelsData.y = accel_rotated[1];
		accelsData.z = accel_rotated[2];
	} else {
		accelsData.x = accels_out[0];
		accelsData.y = accels_out[1];
		accelsData.z = accels_out[2];
	}

	accelsData.z += z_accel_offset;
	accelsData.temperature = accels->temperature;

	AccelsSet(&accelsData);
}

/**
 * @brief Apply calibration and rotation to the raw gyro data
 * @param[in] gyros The raw gyro data
 */
static void update_gyros(struct pios_sensor_gyro_data *gyros)
{
	// Scale the gyros
	float gyros_out[3] = {
	    gyros->x * gyro_scale[0],
	    gyros->y * gyro_scale[1],
	    gyros->z * gyro_scale[2]
	};

	lpfilter_run(gyro_filter, gyros_out);

	GyrosData gyrosData;
	gyrosData.temperature = gyros->temperature;

	// Update the bias due to the temperature
	updateTemperatureComp(gyrosData.temperature, gyro_temp_bias);

	// Apply temperature bias correction before the rotation
	if (bias_correct_gyro) {
		gyros_out[0] -= gyro_temp_bias[0];
		gyros_out[1] -= gyro_temp_bias[1];
		gyros_out[2] -= gyro_temp_bias[2];
	}

	if (rotate) {
		float gyros[3];
		rot_mult(Rsb, gyros_out, gyros, true);
		gyrosData.x = gyros[0];
		gyrosData.y = gyros[1];
		gyrosData.z = gyros[2];
	} else {
		gyrosData.x = gyros_out[0];
		gyrosData.y = gyros_out[1];
		gyrosData.z = gyros_out[2];
	}

	if (bias_correct_gyro) {
		// Apply bias correction to the gyros from the state estimator
		GyrosBiasData gyrosBias;
		GyrosBiasGet(&gyrosBias);
		gyrosData.x -= gyrosBias.x;
		gyrosData.y -= gyrosBias.y;
		gyrosData.z -= gyrosBias.z;

		const float GYRO_BIAS_WARN = 10.0f;
		if (fabsf(gyrosBias.x) > GYRO_BIAS_WARN ||
			fabsf(gyrosBias.y) > GYRO_BIAS_WARN ||
			fabsf(gyrosBias.z) > GYRO_BIAS_WARN) {
			AlarmsSet(SYSTEMALARMS_ALARM_GYROBIAS, SYSTEMALARMS_ALARM_WARNING);
		} else {
			AlarmsClear(SYSTEMALARMS_ALARM_GYROBIAS);
		}
	}

	GyrosSet(&gyrosData);
}

/**
 * @brief Apply calibration and rotation to the raw mag data
 * @param[in] mag The raw mag data
 */
static void update_mags(struct pios_sensor_mag_data *mag)
{
	float mags[3] = {
	    mag->x * mag_scale[0] - mag_bias[0],
	    mag->y * mag_scale[1] - mag_bias[1],
	    mag->z * mag_scale[2] - mag_bias[2]
	};

	MagnetometerData magData;
	if (rotate) {
		float mag_out[3];
		rot_mult(Rsb, mags, mag_out, true);
		magData.x = mag_out[0];
		magData.y = mag_out[1];
		magData.z = mag_out[2];
	} else {
		magData.x = mags[0];
		magData.y = mags[1];
		magData.z = mags[2];
	}

	// Correct for mag bias and update if the rate is non zero
	if (insSettings.MagBiasNullingRate > 0) {
		switch (mag_calibration_algo) {
		case MAG_CALIBRATION_PRELEMARI:
			mag_calibration_prelemari(&magData);
			break;
		case MAG_CALIBRATION_NORMALIZE_LENGTH:
			mag_calibration_fix_length(&magData);
			break;
		default:
			// No calibration
			break;
		}
	}

	MagnetometerSet(&magData);
}

/**
 * Update the baro uavo from the data from the baro queue
 * @param [in] baro raw baro data
 */
static void update_baro(struct pios_sensor_baro_data *baro)
{
	// Check for Nan or infinity
	if (IS_NOT_FINITE(baro->altitude) || IS_NOT_FINITE(baro->temperature) || IS_NOT_FINITE(baro->pressure)) {
		AlarmsSet(SYSTEMALARMS_ALARM_TEMPBARO, SYSTEMALARMS_ALARM_WARNING);
		return;
	}
	
	AlarmsSet(SYSTEMALARMS_ALARM_TEMPBARO, SYSTEMALARMS_ALARM_OK);
	BaroAltitudeData baroAltitude;
	baroAltitude.Temperature = baro->temperature;
	baroAltitude.Pressure = baro->pressure;
	baroAltitude.Altitude = baro->altitude;
	BaroAltitudeSet(&baroAltitude);
}

/*
 * Update the optical flow uavo from the data from the optical flow queue
 * @param [in] optical_flow raw optical flow data
 */
#if defined (PIOS_INCLUDE_OPTICALFLOW)
static void update_optical_flow(struct pios_sensor_optical_flow_data *optical_flow)
{
	OpticalFlowData opticalFlow;

	opticalFlow.x = optical_flow->x_dot;
	opticalFlow.y = optical_flow->y_dot;
	opticalFlow.z = optical_flow->z_dot;

	opticalFlow.Quality = optical_flow->quality;

	OpticalFlowSet(&opticalFlow);
}
#endif /* PIOS_INCLUDE_OPTICALFLOW */

/*
 * Update the rangefinder uavo from the data from the rangefinder queue
 * @param [in] rangefinder raw rangefinder data
 */
#if defined (PIOS_INCLUDE_RANGEFINDER)
static void update_rangefinder(struct pios_sensor_rangefinder_data *data)
{
	RangefinderData rangefinder = { 0 };

	rangefinder.Range = data->range;
	rangefinder.Velocity = data->velocity;

	if (data->range_status) {
		rangefinder.RangingStatus = RANGEFINDER_RANGINGSTATUS_INRANGE;
	} else {
		rangefinder.RangingStatus = RANGEFINDER_RANGINGSTATUS_OUTOFRANGE;
	}

	if (data->velocity_status) {
		rangefinder.VelocityStatus = RANGEFINDER_VELOCITYSTATUS_VALID;
	} else {
		rangefinder.VelocityStatus = RANGEFINDER_VELOCITYSTATUS_INVALID;
	}

	RangefinderSet(&rangefinder);
}
#endif /* PIOS_INCLUDE_RANGEFINDER */

/**
 * Compute the bias expected from temperature variation for each gyro
 * channel
 */
static void updateTemperatureComp(float temperature, float *temp_bias)
{
	static int temp_counter = -1;
	static float temp_accum = 0;
	static const float TEMP_MIN = -10;
	static const float TEMP_MAX = 60;

	if (temperature < TEMP_MIN)
		temperature = TEMP_MIN;
	if (temperature > TEMP_MAX)
		temperature = TEMP_MAX;

	if (temp_counter < 500) {
		temp_accum += temperature;
		temp_counter ++;
	} else {
		float t = temp_accum / temp_counter;
		temp_accum = 0;
		temp_counter = 0;

		// Compute a third order polynomial for each chanel after each 500 samples
		temp_bias[0] = gyro_coeff_x[0] + gyro_coeff_x[1] * t + 
		               gyro_coeff_x[2] * powf(t,2) + gyro_coeff_x[3] * powf(t,3);
		temp_bias[1] = gyro_coeff_y[0] + gyro_coeff_y[1] * t + 
		               gyro_coeff_y[2] * powf(t,2) + gyro_coeff_y[3] * powf(t,3);
		temp_bias[2] = gyro_coeff_z[0] + gyro_coeff_z[1] * t + 
		               gyro_coeff_z[2] * powf(t,2) + gyro_coeff_z[3] * powf(t,3);
	}
}

/**
 * Perform an update of the @ref MagBias based on
 * Magnetometer Offset Cancellation: Theory and Implementation, 
 * revisited William Premerlani, October 14, 2011
 */
static void mag_calibration_prelemari(MagnetometerData *mag)
{
	// Constants, to possibly go into a UAVO
	static const float MIN_NORM_DIFFERENCE = 50;

	static float B2[3] = {0, 0, 0};

	MagBiasData magBias;
	MagBiasGet(&magBias);

	// Remove the current estimate of the bias
	mag->x -= magBias.x;
	mag->y -= magBias.y;
	mag->z -= magBias.z;

	// First call
	if (B2[0] == 0 && B2[1] == 0 && B2[2] == 0) {
		B2[0] = mag->x;
		B2[1] = mag->y;
		B2[2] = mag->z;
		return;
	}

	float B1[3] = {mag->x, mag->y, mag->z};
	float norm_diff = sqrtf(powf(B2[0] - B1[0],2) + powf(B2[1] - B1[1],2) + powf(B2[2] - B1[2],2));
	if (norm_diff > MIN_NORM_DIFFERENCE) {
		float norm_b1 = sqrtf(B1[0]*B1[0] + B1[1]*B1[1] + B1[2]*B1[2]);
		float norm_b2 = sqrtf(B2[0]*B2[0] + B2[1]*B2[1] + B2[2]*B2[2]);
		float scale = insSettings.MagBiasNullingRate * (norm_b2 - norm_b1) / norm_diff;
		float b_error[3] = {(B2[0] - B1[0]) * scale, (B2[1] - B1[1]) * scale, (B2[2] - B1[2]) * scale};

		magBias.x += b_error[0];
		magBias.y += b_error[1];
		magBias.z += b_error[2];

		MagBiasSet(&magBias);

		// Store this value to compare against next update
		B2[0] = B1[0]; B2[1] = B1[1]; B2[2] = B1[2];
	}
}

/**
 * Perform an update of the @ref MagBias based on an algorithm 
 * we developed that tries to drive the magnetometer length to
 * the expected value.  This algorithm seems to work better
 * when not turning a lot.
 */
static void mag_calibration_fix_length(MagnetometerData *mag)
{
	MagBiasData magBias;
	MagBiasGet(&magBias);
	
	// Remove the current estimate of the bias
	mag->x -= magBias.x;
	mag->y -= magBias.y;
	mag->z -= magBias.z;
	
	HomeLocationData homeLocation;
	HomeLocationGet(&homeLocation);
	
	AttitudeActualData attitude;
	AttitudeActualGet(&attitude);
	
	const float Rxy = sqrtf(homeLocation.Be[0]*homeLocation.Be[0] + homeLocation.Be[1]*homeLocation.Be[1]);
	const float Rz = homeLocation.Be[2];
	
	const float rate = insSettings.MagBiasNullingRate;
	float R[3][3];
	float B_e[3];
	float xy[2];
	float delta[3];
	
	// Get the rotation matrix
	Quaternion2R(&attitude.q1, R);
	
	// Rotate the mag into the NED frame
	B_e[0] = R[0][0] * mag->x + R[1][0] * mag->y + R[2][0] * mag->z;
	B_e[1] = R[0][1] * mag->x + R[1][1] * mag->y + R[2][1] * mag->z;
	B_e[2] = R[0][2] * mag->x + R[1][2] * mag->y + R[2][2] * mag->z;
	
	float cy = cosf(attitude.Yaw * DEG2RAD);
	float sy = sinf(attitude.Yaw * DEG2RAD);
	
	xy[0] =  cy * B_e[0] + sy * B_e[1];
	xy[1] = -sy * B_e[0] + cy * B_e[1];
	
	float xy_norm = sqrtf(xy[0]*xy[0] + xy[1]*xy[1]);
	
	delta[0] = -rate * (xy[0] / xy_norm * Rxy - xy[0]);
	delta[1] = -rate * (xy[1] / xy_norm * Rxy - xy[1]);
	delta[2] = -rate * (Rz - B_e[2]);
	
	if (delta[0] == delta[0] && delta[1] == delta[1] && delta[2] == delta[2]) {		
		magBias.x += delta[0];
		magBias.y += delta[1];
		magBias.z += delta[2];
		MagBiasSet(&magBias);
	}
}

/**
 * Locally cache some variables from the AtttitudeSettings object
 */
static void sensors_settings_update()
{
	settings_updated = false;

	SensorSettingsData sensorSettings;
	SensorSettingsGet(&sensorSettings);
	INSSettingsGet(&insSettings);

#ifdef PIOS_TOLERATE_MISSING_SENSORS
	if (sensorSettings.TolerateMissingSensors ==
			SENSORSETTINGS_TOLERATEMISSINGSENSORS_TRUE) {
		missing_sensor_severity = SYSTEMALARMS_ALARM_WARNING;
	} else {
		missing_sensor_severity = SYSTEMALARMS_ALARM_ERROR;
	}
#endif
	
	mag_bias[0] = sensorSettings.MagBias[SENSORSETTINGS_MAGBIAS_X];
	mag_bias[1] = sensorSettings.MagBias[SENSORSETTINGS_MAGBIAS_Y];
	mag_bias[2] = sensorSettings.MagBias[SENSORSETTINGS_MAGBIAS_Z];
	mag_scale[0] = sensorSettings.MagScale[SENSORSETTINGS_MAGSCALE_X];
	mag_scale[1] = sensorSettings.MagScale[SENSORSETTINGS_MAGSCALE_Y];
	mag_scale[2] = sensorSettings.MagScale[SENSORSETTINGS_MAGSCALE_Z];
	accel_bias[0] = sensorSettings.AccelBias[SENSORSETTINGS_ACCELBIAS_X];
	accel_bias[1] = sensorSettings.AccelBias[SENSORSETTINGS_ACCELBIAS_Y];
	accel_bias[2] = sensorSettings.AccelBias[SENSORSETTINGS_ACCELBIAS_Z];
	accel_scale[0] = sensorSettings.AccelScale[SENSORSETTINGS_ACCELSCALE_X];
	accel_scale[1] = sensorSettings.AccelScale[SENSORSETTINGS_ACCELSCALE_Y];
	accel_scale[2] = sensorSettings.AccelScale[SENSORSETTINGS_ACCELSCALE_Z];
	gyro_scale[0] = sensorSettings.GyroScale[SENSORSETTINGS_GYROSCALE_X];
	gyro_scale[1] = sensorSettings.GyroScale[SENSORSETTINGS_GYROSCALE_Y];
	gyro_scale[2] = sensorSettings.GyroScale[SENSORSETTINGS_GYROSCALE_Z];
	gyro_coeff_x[0] =  sensorSettings.XGyroTempCoeff[0];
	gyro_coeff_x[1] =  sensorSettings.XGyroTempCoeff[1];
	gyro_coeff_x[2] =  sensorSettings.XGyroTempCoeff[2];
	gyro_coeff_x[3] =  sensorSettings.XGyroTempCoeff[3];
	gyro_coeff_y[0] =  sensorSettings.YGyroTempCoeff[0];
	gyro_coeff_y[1] =  sensorSettings.YGyroTempCoeff[1];
	gyro_coeff_y[2] =  sensorSettings.YGyroTempCoeff[2];
	gyro_coeff_y[3] =  sensorSettings.YGyroTempCoeff[3];
	gyro_coeff_z[0] =  sensorSettings.ZGyroTempCoeff[0];
	gyro_coeff_z[1] =  sensorSettings.ZGyroTempCoeff[1];
	gyro_coeff_z[2] =  sensorSettings.ZGyroTempCoeff[2];
	gyro_coeff_z[3] =  sensorSettings.ZGyroTempCoeff[3];
	z_accel_offset  =  sensorSettings.ZAccelOffset;

	// Zero out any adaptive tracking
	MagBiasData magBias;
	MagBiasGet(&magBias);
	magBias.x = 0;
	magBias.y = 0;
	magBias.z = 0;
	MagBiasSet(&magBias);

	uint8_t bias_correct;
	AttitudeSettingsBiasCorrectGyroGet(&bias_correct);
	bias_correct_gyro = (bias_correct == ATTITUDESETTINGS_BIASCORRECTGYRO_TRUE);

	AttitudeSettingsData attitudeSettings;
	AttitudeSettingsGet(&attitudeSettings);
	// Indicates not to expend cycles on rotation
	if(attitudeSettings.BoardRotation[0] == 0 && attitudeSettings.BoardRotation[1] == 0 &&
	   attitudeSettings.BoardRotation[2] == 0) {
		rotate = 0;
	} else {
		float rotationQuat[4];
		const float rpy[3] = {attitudeSettings.BoardRotation[ATTITUDESETTINGS_BOARDROTATION_ROLL] / 100.0f,
			attitudeSettings.BoardRotation[ATTITUDESETTINGS_BOARDROTATION_PITCH] / 100.0f,
			attitudeSettings.BoardRotation[ATTITUDESETTINGS_BOARDROTATION_YAW] / 100.0f};
		RPY2Quaternion(rpy, rotationQuat);
		Quaternion2R(rotationQuat, Rsb);
		rotate = 1;
	}

	float gyro_dT = 1.0f / (float)PIOS_SENSORS_GetSampleRate(PIOS_SENSOR_GYRO);
	float accel_dT = 1.0f / (float)PIOS_SENSORS_GetSampleRate(PIOS_SENSOR_ACCEL);

	lpfilter_create(&gyro_filter, sensorSettings.LowpassCutoff, gyro_dT, sensorSettings.LowpassOrder, 3);
	lpfilter_create(&accel_filter, sensorSettings.LowpassCutoff, accel_dT, sensorSettings.LowpassOrder, 3);
}
/**
  * @}
  * @}
  */
