/**
 ******************************************************************************
 * @addtogroup Modules Modules
 * @{
 * @addtogroup Sensors Sensor acquisition module
 * @{
 *
 * @file       sensors.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2016
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Update available sensors registered with @ref PIOS_Sensors
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include <unistd.h>

#include "pios.h"
#include "openpilot.h"
#include "physical_constants.h"
#include "pios_thread.h"

#include "accels.h"
#include "actuatordesired.h"
#include "airspeedactual.h"
#include "attitudeactual.h"
#include "attitudesimulated.h"
#include "attitudesettings.h"
#include "baroairspeed.h"
#include "baroaltitude.h"
#include "gyros.h"
#include "gyrosbias.h"
#include "flightstatus.h"
#include "gpsposition.h"
#include "gpsvelocity.h"
#include "homelocation.h"
#include "magnetometer.h"
#include "magbias.h"
#include "ratedesired.h"
#include "systemsettings.h"

#include "coordinate_conversions.h"

// Private constants
#define STACK_SIZE_BYTES 1540
#define TASK_PRIORITY PIOS_THREAD_PRIO_HIGH
#define SENSOR_PERIOD 2

// Private types

// Private variables
static struct pios_thread *sensorsTaskHandle;

static float accel_bias[3];

static float rand_gauss();

static bool use_real_sensors;
static int sens_rate = 500;

enum sensor_sim_type {MODEL_YASIM, MODEL_QUADCOPTER, MODEL_AIRPLANE, MODEL_CAR} sensor_sim_type;

extern bool use_yasim;

// Private functions
static void SensorsTask(void *parameters);
static void simulateModelQuadcopter();
static void simulateModelAirplane();
static void simulateModelCar();
static void simulateYasim();

static void magOffsetEstimation(MagnetometerData *mag);

extern int32_t SensorsInitialize(void);
extern int32_t SensorsStart(void);

/**
 * Initialise the module.  Called before the start function
 * \returns 0 on success or -1 if initialisation failed
 */
static int32_t SimSensorsInitialize(void)
{
	if (PIOS_SENSORS_IsRegistered(PIOS_SENSOR_GYRO)) {
		use_real_sensors = true;
	}

	if (use_real_sensors) {
		printf("SimSensorsInitialize: Using real sensors!\n");
		return SensorsInitialize();
	}

	printf("SimSensorsInitialize: Using simulated sensors.\n");

	// Register fake address.  Later if we really fake entire sensors then
	// it will make sense to have real queues registered.  For now if these
	// queues are used a crash is appropriate.
	PIOS_SENSORS_Register(PIOS_SENSOR_ACCEL, (struct pios_queue*)1);
	PIOS_SENSORS_Register(PIOS_SENSOR_GYRO, (struct pios_queue*)1);
	PIOS_SENSORS_Register(PIOS_SENSOR_MAG, (struct pios_queue*)1);
	PIOS_SENSORS_Register(PIOS_SENSOR_BARO, (struct pios_queue*)1);

	if (use_yasim) {
		sens_rate = 200;
	}

	PIOS_SENSORS_SetSampleRate(PIOS_SENSOR_ACCEL, sens_rate);
	PIOS_SENSORS_SetSampleRate(PIOS_SENSOR_GYRO, sens_rate);
	PIOS_SENSORS_SetSampleRate(PIOS_SENSOR_MAG, 75);
	PIOS_SENSORS_SetSampleRate(PIOS_SENSOR_BARO, 75);

	accel_bias[0] = rand_gauss() / 10;
	accel_bias[1] = rand_gauss() / 10;
	accel_bias[2] = rand_gauss() / 10;

	AccelsInitialize();
	AttitudeSimulatedInitialize();
	BaroAltitudeInitialize();
	BaroAirspeedInitialize();
	GyrosInitialize();
	GyrosBiasInitialize();
	GPSPositionInitialize();
	GPSVelocityInitialize();
	MagnetometerInitialize();
	MagBiasInitialize();

	return 0;
}

/**
 * Start the task.  Expects all objects to be initialized by this point.
 *pick \returns 0 on success or -1 if initialisation failed
 */
static int32_t SimSensorsStart(void)
{
	if (use_real_sensors) {
		printf("SimSensorsInitialize: starting REAL sensor task\n");
		return SensorsStart();
	}

	printf("SimSensorsInitialize: starting SIMULATED sensor task\n");

	// Watchdog must be registered before starting task
	PIOS_WDG_RegisterFlag(PIOS_WDG_SENSORS);

	// Start main task
	sensorsTaskHandle = PIOS_Thread_Create(SensorsTask, "Sensors", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
	TaskMonitorAdd(TASKINFO_RUNNING_SENSORS, sensorsTaskHandle);

	return 0;
}

MODULE_HIPRI_INITCALL(SimSensorsInitialize, SimSensorsStart)

/**
 * Simulated sensor task.  Run a model of the airframe and produce sensor values
 */
int sensors_count;
static void SensorsTask(void *parameters)
{
	AlarmsClear(SYSTEMALARMS_ALARM_SENSORS);

	PIOS_SENSORS_SetMaxGyro(500);
	// Main task loop
	while (1) {
		PIOS_WDG_UpdateFlag(PIOS_WDG_SENSORS);

		SystemSettingsData systemSettings;
		SystemSettingsGet(&systemSettings);

		switch(systemSettings.AirframeType) {
			case SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWING:
			case SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGELEVON:
			case SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGVTAIL:
				sensor_sim_type = MODEL_AIRPLANE;
				break;
			case SYSTEMSETTINGS_AIRFRAMETYPE_QUADX:
			case SYSTEMSETTINGS_AIRFRAMETYPE_QUADP:
			case SYSTEMSETTINGS_AIRFRAMETYPE_VTOL:
			case SYSTEMSETTINGS_AIRFRAMETYPE_HEXA:
			case SYSTEMSETTINGS_AIRFRAMETYPE_OCTO:
			default:
				sensor_sim_type = MODEL_QUADCOPTER;
				break;
			case SYSTEMSETTINGS_AIRFRAMETYPE_GROUNDVEHICLECAR:
				sensor_sim_type = MODEL_CAR;
				break;
		}

		if (use_yasim) {
			sensor_sim_type = MODEL_YASIM;
		}

		sensors_count++;

		switch(sensor_sim_type) {
			case MODEL_QUADCOPTER:
				simulateModelQuadcopter();
				break;
			case MODEL_AIRPLANE:
				simulateModelAirplane();
				break;
			case MODEL_CAR:
				simulateModelCar();
				break;
			case MODEL_YASIM:
				simulateYasim();
				break;
		}

		static uint32_t tm = 0;

		PIOS_Thread_Sleep_Until(&tm, 1000 / sens_rate);
	}
}

static void simulateYasim()
{
#if !(defined(_WIN32) || defined(WIN32) || defined(__MINGW32__))
	const float GYRO_NOISE_SCALE = 1.0f;
	const float MAG_PERIOD = 1.0 / 75.0;
	const float BARO_PERIOD = 1.0 / 20.0;

	struct command {
	    uint32_t magic;
	    uint32_t flags;

	    float roll, pitch, yaw, throttle;

	    float reserved[8];

	    bool armed;
	} cmd;

	struct status {
	    uint32_t magic;

	    uint32_t flags;

	    double lat, lon, alt;

	    float p, q, r;
	    float acc[3];
	    float vel[3];

	    /* Provided only to "check" attitude solution */
	    float roll, pitch, hdg;

	    float reserved[4];
	} status;

	static bool inited = false;

	static int rfd, wfd;

	if (!inited) {
		fprintf(stderr, "Initing external yasim instance\n");
		inited = true;
		/* pipefd(0) is the read side of pipe */

		int child_in, child_out;

		int pipe_tmp[2];

		int ret = pipe(pipe_tmp);

		if (ret < 0) {
			perror("pipe");
			exit(1);
		}

		child_in = pipe_tmp[0];
		wfd = pipe_tmp[1];

		ret = pipe(pipe_tmp);

		if (ret < 0) {
			perror("pipe");
			exit(1);
		}

		rfd = pipe_tmp[0];
		child_out = pipe_tmp[1];

		ret = fork();

		if (ret < 0) {
			perror("fork");
			exit(1);
		}

		if (ret == 0) {
			/* We are the child */
			if (dup2(child_in, STDIN_FILENO) < 0) {
				perror("dup2STDIN");
				exit(1);
			}

			if (dup2(child_out, STDOUT_FILENO) < 0) {
				perror("dup2STDIN");
				exit(1);
			}

			for (int i = 3; i < 1024; i++) {
				close(i);
			}

			// TODO: Fixup model selection to something nice.
			execlp("yasim-svr", "yasim-svr", "pa22-160-yasim.xml",
					NULL);

			perror("execlp");
			exit(1);
		}

		/* We are the parent. */
		close(child_in);
		close(child_out);
	}

	int ret = read(rfd, &status, sizeof(status));

	if (ret != sizeof(status)) {
		fprintf(stderr, "failed: read-from-yasim %d\n", ret);
		exit(1);
	}

	if (status.magic != 0x00700799) {
		fprintf(stderr, "Bad magic on read from yasim\n");
		exit(1);
	}

	memset(&cmd, 0, sizeof(cmd));

	cmd.magic = 0xb33fbeef;

	FlightStatusData flightStatus;
	FlightStatusGet(&flightStatus);
	ActuatorDesiredData actuatorDesired;
	ActuatorDesiredGet(&actuatorDesired);

	if (isfinite(actuatorDesired.Roll) && isfinite(actuatorDesired.Pitch)
			&& isfinite(actuatorDesired.Yaw)
			&& isfinite(actuatorDesired.Thrust)) {
		cmd.roll = actuatorDesired.Roll;
		cmd.pitch = actuatorDesired.Pitch;
		cmd.yaw = actuatorDesired.Yaw;

		if (actuatorDesired.Thrust >= 0) {
			cmd.throttle = actuatorDesired.Thrust;
		} else {
			cmd.throttle = 0;
		}
	} else {
		/* Work around a bug in attitude where things are going NaN
		 * initially-- still needs troubleshooting.
		 */
		cmd.roll = 0;
		cmd.pitch = 0;
		cmd.yaw = 0;
		cmd.throttle = 0;
	}

	cmd.armed = flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED;

	ret = write(wfd, &cmd, sizeof(cmd));

	if (ret != sizeof(cmd)) {
		fprintf(stderr, "failed: write-to-yasim %d\n", ret);
		exit(1);
	}

	AccelsData accelsData;
	accelsData.x = status.acc[0] + accel_bias[0];
	accelsData.y = status.acc[1] + accel_bias[1];
	accelsData.z = status.acc[2] + accel_bias[2];
	accelsData.temperature = 30;
	AccelsSet(&accelsData);

	GyrosData gyrosData;
	gyrosData.x = (status.p * RAD2DEG) + rand_gauss() * GYRO_NOISE_SCALE;
	gyrosData.y = (status.q * RAD2DEG) + rand_gauss() * GYRO_NOISE_SCALE;
	gyrosData.z = (status.r * RAD2DEG) + rand_gauss() * GYRO_NOISE_SCALE;
	gyrosData.temperature = 31;
	GyrosSet(&gyrosData);

	float rpy[3] = { status.roll * RAD2DEG, status.pitch * RAD2DEG,
		status.hdg * RAD2DEG };

	float q[4];

	RPY2Quaternion(rpy, q);

	float Rbe[3][3];

	Quaternion2R(q, Rbe);

	HomeLocationData homeLocation;
	HomeLocationGet(&homeLocation);
	if (homeLocation.Set == HOMELOCATION_SET_FALSE) {
		homeLocation.Be[0] = 100;
		homeLocation.Be[1] = 0;
		homeLocation.Be[2] = 400;
		homeLocation.Set = HOMELOCATION_SET_TRUE;

		HomeLocationSet(&homeLocation);
	}

	uint32_t last_mag_time = 0;
	if (PIOS_DELAY_DiffuS(last_mag_time) / 1.0e6 > MAG_PERIOD) {
		MagnetometerData mag;
		mag.x = homeLocation.Be[0] * Rbe[0][0] + homeLocation.Be[1] * Rbe[0][1] + homeLocation.Be[2] * Rbe[0][2];
		mag.y = homeLocation.Be[0] * Rbe[1][0] + homeLocation.Be[1] * Rbe[1][1] + homeLocation.Be[2] * Rbe[1][2];
		mag.z = homeLocation.Be[0] * Rbe[2][0] + homeLocation.Be[1] * Rbe[2][1] + homeLocation.Be[2] * Rbe[2][2];

		// Run the offset compensation algorithm from the firmware
		magOffsetEstimation(&mag);

		MagnetometerSet(&mag);
		last_mag_time = PIOS_DELAY_GetRaw();
	}

	static float baro_offset = 0;
	if (baro_offset == 0) {
		// Hacky initialization
		baro_offset = 50;
	} else {
		// Very small drift process
		baro_offset += rand_gauss() / 100;
	}

	// Update baro periodically
	static uint32_t last_baro_time = 0;
	if (PIOS_DELAY_DiffuS(last_baro_time) / 1.0e6 > BARO_PERIOD) {
		BaroAltitudeData baroAltitude;
		BaroAltitudeGet(&baroAltitude);
		baroAltitude.Altitude = -status.alt + baro_offset;
		BaroAltitudeSet(&baroAltitude);
		last_baro_time = PIOS_DELAY_GetRaw();
	}

	AttitudeSimulatedData attitudeSimulated;
	AttitudeSimulatedGet(&attitudeSimulated);
	attitudeSimulated.q1 = q[0];
	attitudeSimulated.q2 = q[1];
	attitudeSimulated.q3 = q[2];
	attitudeSimulated.q4 = q[3];
	Quaternion2RPY(q,&attitudeSimulated.Roll);
#if 0
	attitudeSimulated.Position[0] = pos[0];
	attitudeSimulated.Position[1] = pos[1];
	attitudeSimulated.Position[2] = pos[2];
	attitudeSimulated.Velocity[0] = vel[0];
	attitudeSimulated.Velocity[1] = vel[1];
	attitudeSimulated.Velocity[2] = vel[2];
#endif
	AttitudeSimulatedSet(&attitudeSimulated);

#endif /* !(defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)) */
}

static void simulateModelQuadcopter()
{
	static double pos[3] = {0,0,0};
	static double vel[3] = {0,0,0};
	static double ned_accel[3] = {0,0,0};
	static float q[4] = {1,0,0,0};
	static float rpy[3] = {0,0,0}; // Low pass filtered actuator
	static float baro_offset = 0.0f;
	static float temperature = 20;
	float Rbe[3][3];

	const float ACTUATOR_ALPHA = 0.81f;
	const float MAX_THRUST = GRAVITY * 2;
	const float K_FRICTION = 1;
	const float GPS_PERIOD = 0.1f;
	const float MAG_PERIOD = 1.0 / 75.0;
	const float BARO_PERIOD = 1.0 / 20.0;
	const float GYRO_NOISE_SCALE = 1.0f;

	float dT = 0.002;

	FlightStatusData flightStatus;
	FlightStatusGet(&flightStatus);
	ActuatorDesiredData actuatorDesired;
	ActuatorDesiredGet(&actuatorDesired);

	float thrust = (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) ? actuatorDesired.Thrust * MAX_THRUST : 0;
	if (thrust != thrust)
		thrust = 0;

	float control_scaling = 3000.0f;
	// In rad/s
	rpy[0] = control_scaling * actuatorDesired.Roll * (1 - ACTUATOR_ALPHA) + rpy[0] * ACTUATOR_ALPHA;
	rpy[1] = control_scaling * actuatorDesired.Pitch * (1 - ACTUATOR_ALPHA) + rpy[1] * ACTUATOR_ALPHA;
	rpy[2] = control_scaling * actuatorDesired.Yaw * (1 - ACTUATOR_ALPHA) + rpy[2] * ACTUATOR_ALPHA;

	temperature = 20;
	GyrosData gyrosData; // Skip get as we set all the fields
	gyrosData.x = rpy[0] + rand_gauss() * GYRO_NOISE_SCALE + (temperature - 20) * 1 + powf(temperature - 20,2) * 0.11; // - powf(temperature - 20,3) * 0.05;;
	gyrosData.y = rpy[1] + rand_gauss() * GYRO_NOISE_SCALE + (temperature - 20) * 1 + powf(temperature - 20,2) * 0.11;
	gyrosData.z = rpy[2] + rand_gauss() * GYRO_NOISE_SCALE + (temperature - 20) * 1 + powf(temperature - 20,2) * 0.11;
	gyrosData.temperature = temperature;
	GyrosSet(&gyrosData);

	// Predict the attitude forward in time
	float qdot[4];
	qdot[0] = (-q[1] * rpy[0] - q[2] * rpy[1] - q[3] * rpy[2]) * dT * DEG2RAD / 2;
	qdot[1] = (q[0] * rpy[0] - q[3] * rpy[1] + q[2] * rpy[2]) * dT * DEG2RAD / 2;
	qdot[2] = (q[3] * rpy[0] + q[0] * rpy[1] - q[1] * rpy[2]) * dT * DEG2RAD / 2;
	qdot[3] = (-q[2] * rpy[0] + q[1] * rpy[1] + q[0] * rpy[2]) * dT * DEG2RAD / 2;

	// Take a time step
	q[0] = q[0] + qdot[0];
	q[1] = q[1] + qdot[1];
	q[2] = q[2] + qdot[2];
	q[3] = q[3] + qdot[3];

	float qmag = sqrtf(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
	q[0] = q[0] / qmag;
	q[1] = q[1] / qmag;
	q[2] = q[2] / qmag;
	q[3] = q[3] / qmag;

	static float wind[3] = {0,0,0};
	wind[0] = wind[0] * 0.95 + rand_gauss() / 10.0;
	wind[1] = wind[1] * 0.95 + rand_gauss() / 10.0;
	wind[2] = wind[2] * 0.95 + rand_gauss() / 10.0;

	Quaternion2R(q,Rbe);
	// Make thrust negative as down is positive
	ned_accel[0] = -thrust * Rbe[2][0];
	ned_accel[1] = -thrust * Rbe[2][1];
	// Gravity causes acceleration of 9.81 in the down direction
	ned_accel[2] = -thrust * Rbe[2][2] + GRAVITY;

	// Apply acceleration based on velocity
	ned_accel[0] -= K_FRICTION * (vel[0] - wind[0]);
	ned_accel[1] -= K_FRICTION * (vel[1] - wind[1]);
	ned_accel[2] -= K_FRICTION * (vel[2] - wind[2]);

	// Predict the velocity forward in time
	vel[0] = vel[0] + ned_accel[0] * dT;
	vel[1] = vel[1] + ned_accel[1] * dT;
	vel[2] = vel[2] + ned_accel[2] * dT;

	// Predict the position forward in time
	pos[0] = pos[0] + vel[0] * dT;
	pos[1] = pos[1] + vel[1] * dT;
	pos[2] = pos[2] + vel[2] * dT;

	// Simulate hitting ground
	if(pos[2] > 0) {
		pos[2] = 0;
		vel[2] = 0;
		ned_accel[2] = 0;
	}

	// Sensor feels gravity (when not acceleration in ned frame e.g. ned_accel[2] = 0)
	ned_accel[2] -= GRAVITY;

	// Transform the accels back in to body frame
	AccelsData accelsData; // Skip get as we set all the fields
	accelsData.x = ned_accel[0] * Rbe[0][0] + ned_accel[1] * Rbe[0][1] + ned_accel[2] * Rbe[0][2] + accel_bias[0];
	accelsData.y = ned_accel[0] * Rbe[1][0] + ned_accel[1] * Rbe[1][1] + ned_accel[2] * Rbe[1][2] + accel_bias[1];
	accelsData.z = ned_accel[0] * Rbe[2][0] + ned_accel[1] * Rbe[2][1] + ned_accel[2] * Rbe[2][2] + accel_bias[2];
	accelsData.temperature = 30;
	AccelsSet(&accelsData);

	if(baro_offset == 0) {
		// Hacky initialization
		baro_offset = 50;// * rand_gauss();
	} else {
		// Very small drift process
		baro_offset += rand_gauss() / 100;
	}
	// Update baro periodically
	static uint32_t last_baro_time = 0;
	if(PIOS_DELAY_DiffuS(last_baro_time) / 1.0e6 > BARO_PERIOD) {
		BaroAltitudeData baroAltitude;
		BaroAltitudeGet(&baroAltitude);
		baroAltitude.Altitude = -pos[2] + baro_offset;
		BaroAltitudeSet(&baroAltitude);
		last_baro_time = PIOS_DELAY_GetRaw();
	}

	HomeLocationData homeLocation;
	HomeLocationGet(&homeLocation);
	if (homeLocation.Set == HOMELOCATION_SET_FALSE) {
		homeLocation.Be[0] = 100;
		homeLocation.Be[1] = 0;
		homeLocation.Be[2] = 400;
		homeLocation.Set = HOMELOCATION_SET_TRUE;

		HomeLocationSet(&homeLocation);
	}

	static float gps_vel_drift[3] = {0,0,0};
	gps_vel_drift[0] = gps_vel_drift[0] * 0.65 + rand_gauss() / 5.0;
	gps_vel_drift[1] = gps_vel_drift[1] * 0.65 + rand_gauss() / 5.0;
	gps_vel_drift[2] = gps_vel_drift[2] * 0.65 + rand_gauss() / 5.0;

	// Update GPS periodically
	static uint32_t last_gps_time = 0;
	if(PIOS_DELAY_DiffuS(last_gps_time) / 1.0e6 > GPS_PERIOD) {
		// Use double precision here as simulating what GPS produces
		double T[3];
		T[0] = homeLocation.Altitude+6.378137E6f * DEG2RAD;
		T[1] = cosf(homeLocation.Latitude / 10e6 * DEG2RAD)*(homeLocation.Altitude+6.378137E6) * DEG2RAD;
		T[2] = -1.0;

		static float gps_drift[3] = {0,0,0};
		gps_drift[0] = gps_drift[0] * 0.95 + rand_gauss() / 10.0;
		gps_drift[1] = gps_drift[1] * 0.95 + rand_gauss() / 10.0;
		gps_drift[2] = gps_drift[2] * 0.95 + rand_gauss() / 10.0;

		GPSPositionData gpsPosition;
		GPSPositionGet(&gpsPosition);
		gpsPosition.Latitude = homeLocation.Latitude + ((pos[0] + gps_drift[0]) / T[0] * 10.0e6);
		gpsPosition.Longitude = homeLocation.Longitude + ((pos[1] + gps_drift[1])/ T[1] * 10.0e6);
		gpsPosition.Altitude = homeLocation.Altitude + ((pos[2] + gps_drift[2]) / T[2]);
		gpsPosition.Groundspeed = sqrtf(pow(vel[0] + gps_vel_drift[0],2) + pow(vel[1] + gps_vel_drift[1],2));
		gpsPosition.Heading = 180 / M_PI * atan2f(vel[1] + gps_vel_drift[1],vel[0] + gps_vel_drift[0]);
		gpsPosition.Satellites = 7;
		gpsPosition.PDOP = 1;
		gpsPosition.Accuracy = 3.0;
		gpsPosition.Status = GPSPOSITION_STATUS_FIX3D;
		GPSPositionSet(&gpsPosition);
		last_gps_time = PIOS_DELAY_GetRaw();
	}

	// Update GPS Velocity measurements
	static uint32_t last_gps_vel_time = 1000; // Delay by a millisecond
	if(PIOS_DELAY_DiffuS(last_gps_vel_time) / 1.0e6 > GPS_PERIOD) {
		GPSVelocityData gpsVelocity;
		GPSVelocityGet(&gpsVelocity);
		gpsVelocity.North = vel[0] + gps_vel_drift[0];
		gpsVelocity.East = vel[1] + gps_vel_drift[1];
		gpsVelocity.Down = vel[2] + gps_vel_drift[2];
		gpsVelocity.Accuracy = 0.75;
		GPSVelocitySet(&gpsVelocity);
		last_gps_vel_time = PIOS_DELAY_GetRaw();
	}

	// Update mag periodically
	static uint32_t last_mag_time = 0;
	if(PIOS_DELAY_DiffuS(last_mag_time) / 1.0e6 > MAG_PERIOD) {
		MagnetometerData mag;
		mag.x = homeLocation.Be[0] * Rbe[0][0] + homeLocation.Be[1] * Rbe[0][1] + homeLocation.Be[2] * Rbe[0][2];
		mag.y = homeLocation.Be[0] * Rbe[1][0] + homeLocation.Be[1] * Rbe[1][1] + homeLocation.Be[2] * Rbe[1][2];
		mag.z = homeLocation.Be[0] * Rbe[2][0] + homeLocation.Be[1] * Rbe[2][1] + homeLocation.Be[2] * Rbe[2][2];

		// Run the offset compensation algorithm from the firmware
		magOffsetEstimation(&mag);

		MagnetometerSet(&mag);
		last_mag_time = PIOS_DELAY_GetRaw();
	}

	AttitudeSimulatedData attitudeSimulated;
	AttitudeSimulatedGet(&attitudeSimulated);
	attitudeSimulated.q1 = q[0];
	attitudeSimulated.q2 = q[1];
	attitudeSimulated.q3 = q[2];
	attitudeSimulated.q4 = q[3];
	Quaternion2RPY(q,&attitudeSimulated.Roll);
	attitudeSimulated.Position[0] = pos[0];
	attitudeSimulated.Position[1] = pos[1];
	attitudeSimulated.Position[2] = pos[2];
	attitudeSimulated.Velocity[0] = vel[0];
	attitudeSimulated.Velocity[1] = vel[1];
	attitudeSimulated.Velocity[2] = vel[2];
	AttitudeSimulatedSet(&attitudeSimulated);
}

/**
 * This method performs a simple simulation of an airplane
 * 
 * It takes in the ActuatorDesired command to rotate the aircraft and performs
 * a simple kinetic model where the throttle increases the energy and drag decreases
 * it.  Changing altitude moves energy from kinetic to potential.
 *
 * 1. Update attitude based on ActuatorDesired
 * 2. Update position based on velocity
 */
static void simulateModelAirplane()
{
	static double pos[3] = {0,0,0};
	static double vel[3] = {0,0,0};
	static double ned_accel[3] = {0,0,0};
	static float q[4] = {1,0,0,0};
	static float rpy[3] = {0,0,0}; // Low pass filtered actuator
	static float baro_offset = 0.0f;
	float Rbe[3][3];

	const float LIFT_SPEED = 8; // (m/s) where achieve lift for zero pitch
	const float ACTUATOR_ALPHA = 0.8;
	const float MAX_THRUST = 9.81 * 2;
	const float K_FRICTION = 0.2;
	const float GPS_PERIOD = 0.1;
	const float MAG_PERIOD = 1.0 / 75.0;
	const float BARO_PERIOD = 1.0 / 20.0;
	const float ROLL_HEADING_COUPLING = 0.1; // (deg/s) heading change per deg of roll
	const float PITCH_THRUST_COUPLING = 0.2; // (m/s^2) of forward acceleration per deg of pitch

	float dT = 0.002;

	FlightStatusData flightStatus;
	FlightStatusGet(&flightStatus);
	ActuatorDesiredData actuatorDesired;
	ActuatorDesiredGet(&actuatorDesired);

	float thrust = (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) ? actuatorDesired.Thrust * MAX_THRUST : 0;

	if (thrust != thrust)
		thrust = 0;

	/**** 1. Update attitude ****/
	RateDesiredData rateDesired;
	RateDesiredGet(&rateDesired);

	// Need to get roll angle for easy cross coupling
	AttitudeActualData attitudeActual;
	AttitudeActualGet(&attitudeActual);
	double roll = attitudeActual.Roll;
	double pitch = attitudeActual.Pitch;

	rpy[0] = (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) * rateDesired.Roll * (1 - ACTUATOR_ALPHA) + rpy[0] * ACTUATOR_ALPHA;
	rpy[1] = (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) * rateDesired.Pitch * (1 - ACTUATOR_ALPHA) + rpy[1] * ACTUATOR_ALPHA;
	rpy[2] = (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) * rateDesired.Yaw * (1 - ACTUATOR_ALPHA) + rpy[2] * ACTUATOR_ALPHA;
	rpy[2] += roll * ROLL_HEADING_COUPLING;


	GyrosData gyrosData; // Skip get as we set all the fields
	gyrosData.x = rpy[0] + rand_gauss();
	gyrosData.y = rpy[1] + rand_gauss();
	gyrosData.z = rpy[2] + rand_gauss();
	GyrosSet(&gyrosData);

	// Predict the attitude forward in time
	float qdot[4];
	qdot[0] = (-q[1] * rpy[0] - q[2] * rpy[1] - q[3] * rpy[2]) * dT * DEG2RAD / 2;
	qdot[1] = (q[0] * rpy[0] - q[3] * rpy[1] + q[2] * rpy[2]) * dT * DEG2RAD / 2;
	qdot[2] = (q[3] * rpy[0] + q[0] * rpy[1] - q[1] * rpy[2]) * dT * DEG2RAD / 2;
	qdot[3] = (-q[2] * rpy[0] + q[1] * rpy[1] + q[0] * rpy[2]) * dT * DEG2RAD / 2;

	// Take a time step
	q[0] = q[0] + qdot[0];
	q[1] = q[1] + qdot[1];
	q[2] = q[2] + qdot[2];
	q[3] = q[3] + qdot[3];

	float qmag = sqrtf(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
	q[0] = q[0] / qmag;
	q[1] = q[1] / qmag;
	q[2] = q[2] / qmag;
	q[3] = q[3] / qmag;

	/**** 2. Update position based on velocity ****/
	static float wind[3] = {0,0,0};
	wind[0] = wind[0] * 0.95 + rand_gauss() / 10.0;
	wind[1] = wind[1] * 0.95 + rand_gauss() / 10.0;
	wind[2] = wind[2] * 0.95 + rand_gauss() / 10.0;
	wind[0] = 0;
	wind[1] = 0;
	wind[2] = 0;

	// Rbe takes a vector from body to earth.  If we take (1,0,0)^T through this and then dot with airspeed
	// we get forward airspeed
	Quaternion2R(q,Rbe);

	double airspeed[3] = {vel[0] - wind[0], vel[1] - wind[1], vel[2] - wind[2]};
	double forwardAirspeed = Rbe[0][0] * airspeed[0] + Rbe[0][1] * airspeed[1] + Rbe[0][2] * airspeed[2];
	double sidewaysAirspeed = Rbe[1][0] * airspeed[0] + Rbe[1][1] * airspeed[1] + Rbe[1][2] * airspeed[2];
	double downwardAirspeed = Rbe[2][0] * airspeed[0] + Rbe[2][1] * airspeed[1] + Rbe[2][2] * airspeed[2];

	AirspeedActualData airspeedObj;
	airspeedObj.CalibratedAirspeed = forwardAirspeed;
	// TODO: Factor in temp and pressure when simulated for true airspeed.
	// This assume standard temperature and pressure which will be inaccurate
	// at higher altitudes (http://en.wikipedia.org/wiki/Airspeed)
	airspeedObj.TrueAirspeed = forwardAirspeed;
	AirspeedActualSet(&airspeedObj);

	/* Compute aerodynamic forces in body referenced frame.  Later use more sophisticated equations  */
	/* TODO: This should become more accurate.  Use the force equations to calculate lift from the   */
	/* various surfaces based on AoA and airspeed.  From that compute torques and forces.  For later */
	double forces[3]; // X, Y, Z
	forces[0] = thrust - pitch * PITCH_THRUST_COUPLING - forwardAirspeed * K_FRICTION;         // Friction is applied in all directions in NED
	forces[1] = 0 - sidewaysAirspeed * K_FRICTION * 100;      // No side slip
	forces[2] = GRAVITY * (forwardAirspeed - LIFT_SPEED) + downwardAirspeed * K_FRICTION * 100;    // Stupidly simple, always have gravity lift when straight and level

	// Negate force[2] as NED defines down as possitive, aircraft convention is Z up is positive (?)
	ned_accel[0] = forces[0] * Rbe[0][0] + forces[1] * Rbe[1][0] - forces[2] * Rbe[2][0];
	ned_accel[1] = forces[0] * Rbe[0][1] + forces[1] * Rbe[1][1] - forces[2] * Rbe[2][1];
	ned_accel[2] = forces[0] * Rbe[0][2] + forces[1] * Rbe[1][2] - forces[2] * Rbe[2][2];
	// Gravity causes acceleration of 9.81 in the down direction
	ned_accel[2] += 9.81;

	// Apply acceleration based on velocity
	ned_accel[0] -= K_FRICTION * (vel[0] - wind[0]);
	ned_accel[1] -= K_FRICTION * (vel[1] - wind[1]);
	ned_accel[2] -= K_FRICTION * (vel[2] - wind[2]);

	// Predict the velocity forward in time
	vel[0] = vel[0] + ned_accel[0] * dT;
	vel[1] = vel[1] + ned_accel[1] * dT;
	vel[2] = vel[2] + ned_accel[2] * dT;

	// Predict the position forward in time
	pos[0] = pos[0] + vel[0] * dT;
	pos[1] = pos[1] + vel[1] * dT;
	pos[2] = pos[2] + vel[2] * dT;

	// Simulate hitting ground
	if(pos[2] > 0) {
		pos[2] = 0;
		vel[2] = 0;
		ned_accel[2] = 0;
	}

	// Sensor feels gravity (when not acceleration in ned frame e.g. ned_accel[2] = 0)
	ned_accel[2] -= GRAVITY;

	// Transform the accels back in to body frame
	AccelsData accelsData; // Skip get as we set all the fields
	accelsData.x = ned_accel[0] * Rbe[0][0] + ned_accel[1] * Rbe[0][1] + ned_accel[2] * Rbe[0][2] + accel_bias[0];
	accelsData.y = ned_accel[0] * Rbe[1][0] + ned_accel[1] * Rbe[1][1] + ned_accel[2] * Rbe[1][2] + accel_bias[1];
	accelsData.z = ned_accel[0] * Rbe[2][0] + ned_accel[1] * Rbe[2][1] + ned_accel[2] * Rbe[2][2] + accel_bias[2];
	accelsData.temperature = 30;
	AccelsSet(&accelsData);

	if(baro_offset == 0) {
		// Hacky initialization
		baro_offset = 50;// * rand_gauss();
	} else {
		// Very small drift process
		baro_offset += rand_gauss() / 100;
	}
	// Update baro periodically
	static uint32_t last_baro_time = 0;
	if(PIOS_DELAY_DiffuS(last_baro_time) / 1.0e6 > BARO_PERIOD) {
		BaroAltitudeData baroAltitude;
		BaroAltitudeGet(&baroAltitude);
		baroAltitude.Altitude = -pos[2] + baro_offset;
		BaroAltitudeSet(&baroAltitude);
		last_baro_time = PIOS_DELAY_GetRaw();
	}

	// Update baro airpseed periodically
	static uint32_t last_airspeed_time = 0;
	if(PIOS_DELAY_DiffuS(last_airspeed_time) / 1.0e6 > BARO_PERIOD) {
		BaroAirspeedData baroAirspeed;
		baroAirspeed.BaroConnected = BAROAIRSPEED_BAROCONNECTED_TRUE;
		baroAirspeed.CalibratedAirspeed = forwardAirspeed;
		baroAirspeed.GPSAirspeed = forwardAirspeed;
		baroAirspeed.TrueAirspeed = forwardAirspeed;
		BaroAirspeedSet(&baroAirspeed);
		last_airspeed_time = PIOS_DELAY_GetRaw();
	}

	HomeLocationData homeLocation;
	HomeLocationGet(&homeLocation);
	if (homeLocation.Set == HOMELOCATION_SET_FALSE) {
		homeLocation.Be[0] = 100;
		homeLocation.Be[1] = 0;
		homeLocation.Be[2] = 400;
		homeLocation.Set = HOMELOCATION_SET_TRUE;

		HomeLocationSet(&homeLocation);
	}

	static float gps_vel_drift[3] = {0,0,0};
	gps_vel_drift[0] = gps_vel_drift[0] * 0.65 + rand_gauss() / 5.0;
	gps_vel_drift[1] = gps_vel_drift[1] * 0.65 + rand_gauss() / 5.0;
	gps_vel_drift[2] = gps_vel_drift[2] * 0.65 + rand_gauss() / 5.0;

	// Update GPS periodically
	static uint32_t last_gps_time = 0;
	if(PIOS_DELAY_DiffuS(last_gps_time) / 1.0e6 > GPS_PERIOD) {
		// Use double precision here as simulating what GPS produces
		double T[3];
		T[0] = homeLocation.Altitude+6.378137E6f * DEG2RAD;
		T[1] = cosf(homeLocation.Latitude / 10e6 * DEG2RAD)*(homeLocation.Altitude+6.378137E6) * DEG2RAD;
		T[2] = -1.0;

		static float gps_drift[3] = {0,0,0};
		gps_drift[0] = gps_drift[0] * 0.95 + rand_gauss() / 10.0;
		gps_drift[1] = gps_drift[1] * 0.95 + rand_gauss() / 10.0;
		gps_drift[2] = gps_drift[2] * 0.95 + rand_gauss() / 10.0;

		GPSPositionData gpsPosition;
		GPSPositionGet(&gpsPosition);
		gpsPosition.Latitude = homeLocation.Latitude + ((pos[0] + gps_drift[0]) / T[0] * 10.0e6);
		gpsPosition.Longitude = homeLocation.Longitude + ((pos[1] + gps_drift[1])/ T[1] * 10.0e6);
		gpsPosition.Altitude = homeLocation.Altitude + ((pos[2] + gps_drift[2]) / T[2]);
		gpsPosition.Groundspeed = sqrtf(pow(vel[0] + gps_vel_drift[0],2) + pow(vel[1] + gps_vel_drift[1],2));
		gpsPosition.Heading = 180 / M_PI * atan2f(vel[1] + gps_vel_drift[1],vel[0] + gps_vel_drift[0]);
		gpsPosition.Satellites = 7;
		gpsPosition.PDOP = 1;
		GPSPositionSet(&gpsPosition);
		last_gps_time = PIOS_DELAY_GetRaw();
	}

	// Update GPS Velocity measurements
	static uint32_t last_gps_vel_time = 1000; // Delay by a millisecond
	if(PIOS_DELAY_DiffuS(last_gps_vel_time) / 1.0e6 > GPS_PERIOD) {
		GPSVelocityData gpsVelocity;
		GPSVelocityGet(&gpsVelocity);
		gpsVelocity.North = vel[0] + gps_vel_drift[0];
		gpsVelocity.East = vel[1] + gps_vel_drift[1];
		gpsVelocity.Down = vel[2] + gps_vel_drift[2];
		GPSVelocitySet(&gpsVelocity);
		last_gps_vel_time = PIOS_DELAY_GetRaw();
	}

	// Update mag periodically
	static uint32_t last_mag_time = 0;
	if(PIOS_DELAY_DiffuS(last_mag_time) / 1.0e6 > MAG_PERIOD) {
		MagnetometerData mag;
		mag.x = 100+homeLocation.Be[0] * Rbe[0][0] + homeLocation.Be[1] * Rbe[0][1] + homeLocation.Be[2] * Rbe[0][2];
		mag.y = 100+homeLocation.Be[0] * Rbe[1][0] + homeLocation.Be[1] * Rbe[1][1] + homeLocation.Be[2] * Rbe[1][2];
		mag.z = 100+homeLocation.Be[0] * Rbe[2][0] + homeLocation.Be[1] * Rbe[2][1] + homeLocation.Be[2] * Rbe[2][2];
		magOffsetEstimation(&mag);
		MagnetometerSet(&mag);
		last_mag_time = PIOS_DELAY_GetRaw();
	}

	AttitudeSimulatedData attitudeSimulated;
	AttitudeSimulatedGet(&attitudeSimulated);
	attitudeSimulated.q1 = q[0];
	attitudeSimulated.q2 = q[1];
	attitudeSimulated.q3 = q[2];
	attitudeSimulated.q4 = q[3];
	Quaternion2RPY(q,&attitudeSimulated.Roll);
	attitudeSimulated.Position[0] = pos[0];
	attitudeSimulated.Position[1] = pos[1];
	attitudeSimulated.Position[2] = pos[2];
	attitudeSimulated.Velocity[0] = vel[0];
	attitudeSimulated.Velocity[1] = vel[1];
	attitudeSimulated.Velocity[2] = vel[2];
	AttitudeSimulatedSet(&attitudeSimulated);
}

/**
 * This method performs a simple simulation of a car
 * 
 * It takes in the ActuatorDesired command to rotate the aircraft and performs
 * a simple kinetic model where the throttle increases the energy and drag decreases
 * it.  Changing altitude moves energy from kinetic to potential.
 *
 * 1. Update attitude based on ActuatorDesired
 * 2. Update position based on velocity
 */
static void simulateModelCar()
{
	static double pos[3] = {0,0,0};
	static double vel[3] = {0,0,0};
	static double ned_accel[3] = {0,0,0};
	static float q[4] = {1,0,0,0};
	static float rpy[3] = {0,0,0}; // Low pass filtered actuator
	static float baro_offset = 0.0f;
	float Rbe[3][3];

	const float ACTUATOR_ALPHA = 0.8;
	const float MAX_THRUST = 9.81 * 0.5;
	const float K_FRICTION = 0.2;
	const float GPS_PERIOD = 0.1;
	const float MAG_PERIOD = 1.0 / 75.0;
	const float BARO_PERIOD = 1.0 / 20.0;

	float dT = 0.002;

	FlightStatusData flightStatus;
	FlightStatusGet(&flightStatus);
	ActuatorDesiredData actuatorDesired;
	ActuatorDesiredGet(&actuatorDesired);

	float thrust = (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) ? actuatorDesired.Thrust * MAX_THRUST : 0;

	if (thrust != thrust)
		thrust = 0;

	/**** 1. Update attitude ****/
	RateDesiredData rateDesired;
	RateDesiredGet(&rateDesired);

	// Need to get roll angle for easy cross coupling
	AttitudeActualData attitudeActual;
	AttitudeActualGet(&attitudeActual);

	rpy[0] = 0; // cannot roll
	rpy[1] = 0; // cannot pitch
	rpy[2] = (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) * rateDesired.Yaw * (1 - ACTUATOR_ALPHA) + rpy[2] * ACTUATOR_ALPHA;


	GyrosData gyrosData; // Skip get as we set all the fields
	gyrosData.x = rpy[0] + rand_gauss();
	gyrosData.y = rpy[1] + rand_gauss();
	gyrosData.z = rpy[2] + rand_gauss();
	GyrosSet(&gyrosData);

	// Predict the attitude forward in time
	float qdot[4];
	qdot[0] = (-q[1] * rpy[0] - q[2] * rpy[1] - q[3] * rpy[2]) * dT * DEG2RAD / 2;
	qdot[1] = (q[0] * rpy[0] - q[3] * rpy[1] + q[2] * rpy[2]) * dT * DEG2RAD / 2;
	qdot[2] = (q[3] * rpy[0] + q[0] * rpy[1] - q[1] * rpy[2]) * dT * DEG2RAD / 2;
	qdot[3] = (-q[2] * rpy[0] + q[1] * rpy[1] + q[0] * rpy[2]) * dT * DEG2RAD / 2;

	// Take a time step
	q[0] = q[0] + qdot[0];
	q[1] = q[1] + qdot[1];
	q[2] = q[2] + qdot[2];
	q[3] = q[3] + qdot[3];

	float qmag = sqrtf(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
	q[0] = q[0] / qmag;
	q[1] = q[1] / qmag;
	q[2] = q[2] / qmag;
	q[3] = q[3] / qmag;

	/**** 2. Update position based on velocity ****/
	// Rbe takes a vector from body to earth.  If we take (1,0,0)^T through this and then dot with airspeed
	// we get forward airspeed
	Quaternion2R(q,Rbe);

	double groundspeed[3] = {vel[0], vel[1], vel[2] };
	double forwardSpeed = Rbe[0][0] * groundspeed[0] + Rbe[0][1] * groundspeed[1] + Rbe[0][2] * groundspeed[2];
	double sidewaysSpeed = Rbe[1][0] * groundspeed[0] + Rbe[1][1] * groundspeed[1] + Rbe[1][2] * groundspeed[2];

	double forces[3]; // X, Y, Z
	forces[0] = thrust - forwardSpeed * K_FRICTION;         // Friction is applied in all directions in NED
	forces[1] = 0 - sidewaysSpeed * K_FRICTION * 100;      // No side slip
	forces[2] = 0;

	// Negate force[2] as NED defines down as possitive, aircraft convention is Z up is positive (?)
	ned_accel[0] = forces[0] * Rbe[0][0] + forces[1] * Rbe[1][0] - forces[2] * Rbe[2][0];
	ned_accel[1] = forces[0] * Rbe[0][1] + forces[1] * Rbe[1][1] - forces[2] * Rbe[2][1];
	ned_accel[2] = 0;

	// Apply acceleration based on velocity
	ned_accel[0] -= K_FRICTION * (vel[0]);
	ned_accel[1] -= K_FRICTION * (vel[1]);

	// Predict the velocity forward in time
	vel[0] = vel[0] + ned_accel[0] * dT;
	vel[1] = vel[1] + ned_accel[1] * dT;
	vel[2] = vel[2] + ned_accel[2] * dT;

	// Predict the position forward in time
	pos[0] = pos[0] + vel[0] * dT;
	pos[1] = pos[1] + vel[1] * dT;
	pos[2] = pos[2] + vel[2] * dT;

	// Simulate hitting ground
	if(pos[2] > 0) {
		pos[2] = 0;
		vel[2] = 0;
		ned_accel[2] = 0;
	}

	// Sensor feels gravity (when not acceleration in ned frame e.g. ned_accel[2] = 0)
	ned_accel[2] -= GRAVITY;

	// Transform the accels back in to body frame
	AccelsData accelsData; // Skip get as we set all the fields
	accelsData.x = ned_accel[0] * Rbe[0][0] + ned_accel[1] * Rbe[0][1] + ned_accel[2] * Rbe[0][2] + accel_bias[0];
	accelsData.y = ned_accel[0] * Rbe[1][0] + ned_accel[1] * Rbe[1][1] + ned_accel[2] * Rbe[1][2] + accel_bias[1];
	accelsData.z = ned_accel[0] * Rbe[2][0] + ned_accel[1] * Rbe[2][1] + ned_accel[2] * Rbe[2][2] + accel_bias[2];
	accelsData.temperature = 30;
	AccelsSet(&accelsData);

	if(baro_offset == 0) {
		// Hacky initialization
		baro_offset = 50;// * rand_gauss();
	} else {
		// Very small drift process
		baro_offset += rand_gauss() / 100;
	}
	// Update baro periodically
	static uint32_t last_baro_time = 0;
	if(PIOS_DELAY_DiffuS(last_baro_time) / 1.0e6 > BARO_PERIOD) {
		BaroAltitudeData baroAltitude;
		BaroAltitudeGet(&baroAltitude);
		baroAltitude.Altitude = -pos[2] + baro_offset;
		BaroAltitudeSet(&baroAltitude);
		last_baro_time = PIOS_DELAY_GetRaw();
	}

	HomeLocationData homeLocation;
	HomeLocationGet(&homeLocation);
	if (homeLocation.Set == HOMELOCATION_SET_FALSE) {
		homeLocation.Be[0] = 100;
		homeLocation.Be[1] = 0;
		homeLocation.Be[2] = 400;
		homeLocation.Set = HOMELOCATION_SET_TRUE;

		HomeLocationSet(&homeLocation);
	}

	static float gps_vel_drift[3] = {0,0,0};
	gps_vel_drift[0] = gps_vel_drift[0] * 0.65 + rand_gauss() / 5.0;
	gps_vel_drift[1] = gps_vel_drift[1] * 0.65 + rand_gauss() / 5.0;
	gps_vel_drift[2] = gps_vel_drift[2] * 0.65 + rand_gauss() / 5.0;

	// Update GPS periodically
	static uint32_t last_gps_time = 0;
	if(PIOS_DELAY_DiffuS(last_gps_time) / 1.0e6 > GPS_PERIOD) {
		// Use double precision here as simulating what GPS produces
		double T[3];
		T[0] = homeLocation.Altitude+6.378137E6f * DEG2RAD;
		T[1] = cosf(homeLocation.Latitude / 10e6 * DEG2RAD)*(homeLocation.Altitude+6.378137E6) * DEG2RAD;
		T[2] = -1.0;

		static float gps_drift[3] = {0,0,0};
		gps_drift[0] = gps_drift[0] * 0.95 + rand_gauss() / 10.0;
		gps_drift[1] = gps_drift[1] * 0.95 + rand_gauss() / 10.0;
		gps_drift[2] = gps_drift[2] * 0.95 + rand_gauss() / 10.0;

		GPSPositionData gpsPosition;
		GPSPositionGet(&gpsPosition);
		gpsPosition.Latitude = homeLocation.Latitude + ((pos[0] + gps_drift[0]) / T[0] * 10.0e6);
		gpsPosition.Longitude = homeLocation.Longitude + ((pos[1] + gps_drift[1])/ T[1] * 10.0e6);
		gpsPosition.Altitude = homeLocation.Altitude + ((pos[2] + gps_drift[2]) / T[2]);
		gpsPosition.Groundspeed = sqrtf(pow(vel[0] + gps_vel_drift[0],2) + pow(vel[1] + gps_vel_drift[1],2));
		gpsPosition.Heading = 180 / M_PI * atan2f(vel[1] + gps_vel_drift[1],vel[0] + gps_vel_drift[0]);
		gpsPosition.Satellites = 7;
		gpsPosition.PDOP = 1;
		GPSPositionSet(&gpsPosition);
		last_gps_time = PIOS_DELAY_GetRaw();
	}

	// Update GPS Velocity measurements
	static uint32_t last_gps_vel_time = 1000; // Delay by a millisecond
	if(PIOS_DELAY_DiffuS(last_gps_vel_time) / 1.0e6 > GPS_PERIOD) {
		GPSVelocityData gpsVelocity;
		GPSVelocityGet(&gpsVelocity);
		gpsVelocity.North = vel[0] + gps_vel_drift[0];
		gpsVelocity.East = vel[1] + gps_vel_drift[1];
		gpsVelocity.Down = vel[2] + gps_vel_drift[2];
		GPSVelocitySet(&gpsVelocity);
		last_gps_vel_time = PIOS_DELAY_GetRaw();
	}

	// Update mag periodically
	static uint32_t last_mag_time = 0;
	if(PIOS_DELAY_DiffuS(last_mag_time) / 1.0e6 > MAG_PERIOD) {
		MagnetometerData mag;
		mag.x = 100+homeLocation.Be[0] * Rbe[0][0] + homeLocation.Be[1] * Rbe[0][1] + homeLocation.Be[2] * Rbe[0][2];
		mag.y = 100+homeLocation.Be[0] * Rbe[1][0] + homeLocation.Be[1] * Rbe[1][1] + homeLocation.Be[2] * Rbe[1][2];
		mag.z = 100+homeLocation.Be[0] * Rbe[2][0] + homeLocation.Be[1] * Rbe[2][1] + homeLocation.Be[2] * Rbe[2][2];
		magOffsetEstimation(&mag);
		MagnetometerSet(&mag);
		last_mag_time = PIOS_DELAY_GetRaw();
	}

	AttitudeSimulatedData attitudeSimulated;
	AttitudeSimulatedGet(&attitudeSimulated);
	attitudeSimulated.q1 = q[0];
	attitudeSimulated.q2 = q[1];
	attitudeSimulated.q3 = q[2];
	attitudeSimulated.q4 = q[3];
	Quaternion2RPY(q,&attitudeSimulated.Roll);
	attitudeSimulated.Position[0] = pos[0];
	attitudeSimulated.Position[1] = pos[1];
	attitudeSimulated.Position[2] = pos[2];
	attitudeSimulated.Velocity[0] = vel[0];
	attitudeSimulated.Velocity[1] = vel[1];
	attitudeSimulated.Velocity[2] = vel[2];
	AttitudeSimulatedSet(&attitudeSimulated);
}


static float rand_gauss (void) {
	float v1,v2,s;

	do {
		v1 = 2.0 * ((float) rand()/RAND_MAX) - 1;
		v2 = 2.0 * ((float) rand()/RAND_MAX) - 1;

		s = v1*v1 + v2*v2;
	} while ( s >= 1.0 );

	if (s == 0.0)
		return 0.0;
	else
		return (v1*sqrtf(-2.0 * log(s) / s));
}

/**
 * Perform an update of the @ref MagBias based on
 * Magnetometer Offset Cancellation: Theory and Implementation, 
 * revisited William Premerlani, October 14, 2011
 */
static void magOffsetEstimation(MagnetometerData *mag)
{
	HomeLocationData homeLocation;
	HomeLocationGet(&homeLocation);

	AttitudeActualData attitude;
	AttitudeActualGet(&attitude);

	MagBiasData magBias;
	MagBiasGet(&magBias);

	// Remove the current estimate of the bias
	mag->x -= magBias.x;
	mag->y -= magBias.y;
	mag->z -= magBias.z;

	const float Rxy = sqrtf(homeLocation.Be[0]*homeLocation.Be[0] + homeLocation.Be[1]*homeLocation.Be[1]);
	const float Rz = homeLocation.Be[2];

	const float rate = 0.01;
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

	if (xy_norm > 0) {
		delta[0] = -rate * (xy[0] / xy_norm * Rxy - xy[0]);
		delta[1] = -rate * (xy[1] / xy_norm * Rxy - xy[1]);
		delta[2] = -rate * (Rz - B_e[2]);

		magBias.x += delta[0];
		magBias.y += delta[1];
		magBias.z += delta[2];
		MagBiasSet(&magBias);
	}
}

/**
  * @}
  * @}
  */
