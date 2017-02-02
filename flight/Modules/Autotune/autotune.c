/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup AutotuningModule Autotuning Module
 * @{
 *
 * @file       autotune.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2016
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      State machine to run autotuning. Low level work done by @ref
 *             StabilizationModule
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

#include "openpilot.h"
#include "pios.h"
#include "physical_constants.h"
#include "flightstatus.h"
#include "modulesettings.h"
#include "manualcontrolcommand.h"
#include "manualcontrolsettings.h"
#include "gyros.h"
#include "actuatordesired.h"
#include "stabilizationdesired.h"
#include "stabilizationsettings.h"
#include "systemident.h"
#include <pios_board_info.h>
#include "pios_thread.h"
#include "systemsettings.h"

#include "circqueue.h"
#include "misc_math.h"

// Private constants
#define STACK_SIZE_BYTES 1340
#define TASK_PRIORITY PIOS_THREAD_PRIO_NORMAL

#define AF_NUMX 13
#define AF_NUMP 43

// Private types
enum AUTOTUNE_STATE { AT_INIT, AT_START, AT_WAITFIRSTPOINT, AT_RUN,
	AT_WAITING };

#define THROTTLE_EOF	1000.0f	/* This value doesn't make sense, so it's
				 * a good end-of-actuation indicator.
				 * (normal range [-1, 1])
				 */

#define ATFLASH_MAGIC 0x656e755480008041	/* A...Tune */
struct at_flash_header {
	uint64_t magic;
	uint16_t wiggle_points;
	uint16_t aux_data_len;
	uint16_t sample_rate;

	// Consider total number of averages here
	uint16_t resv;
};

struct at_measurement {
	float y[3];		/* Gyro measurements */
	float u[3];		/* Actuator desired */
};

struct at_queued_data {
	struct at_measurement meas;
	float throttle;		/* Throttle desired */

	uint16_t sample_num;
};

#ifdef AUTOTUNE_AVERAGING_MODE
static struct at_measurement *at_averages;
#endif

// Private variables
static struct pios_thread *taskHandle;
static bool module_enabled;
static circ_queue_t at_queue;
static volatile uint32_t at_points_spilled;
static uint32_t throttle_accumulator;

extern uint16_t ident_wiggle_points;

// Private functions
static void AutotuneTask(void *parameters);
static void af_predict(float X[AF_NUMX], float P[AF_NUMP], const struct at_measurement *measurement, const float dT_s, const float t_in);
static void af_init(float X[AF_NUMX], float P[AF_NUMP]);

#ifndef AT_QUEUE_NUMELEM

#ifdef SMALLF1
#define AT_QUEUE_NUMELEM 19
#else
#define AT_QUEUE_NUMELEM 32
#endif

#endif

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AutotuneInitialize(void)
{
#ifndef SMALLF1
	if (SystemIdentInitialize() == -1) {
		module_enabled = false;
		return -1;
	}
#endif

	// Create a queue, connect to manual control command and flightstatus
#ifdef MODULE_Autotune_BUILTIN
	module_enabled = true;
#else
	uint8_t module_state[MODULESETTINGS_ADMINSTATE_NUMELEM];
	ModuleSettingsAdminStateGet(module_state);
	if (module_state[MODULESETTINGS_ADMINSTATE_AUTOTUNE] == MODULESETTINGS_ADMINSTATE_ENABLED)
		module_enabled = true;
	else
		module_enabled = false;
#endif

	if (module_enabled) {
		if (SystemIdentInitialize() == -1) {
			module_enabled = false;
			return -1;
		}

		at_queue = circ_queue_new(sizeof(struct at_queued_data),
				AT_QUEUE_NUMELEM);

		if (!at_queue)
			module_enabled = false;
	}

	return 0;
}

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AutotuneStart(void)
{
	// Start main task if it is enabled
	if(module_enabled) {
		// Start main task
		taskHandle = PIOS_Thread_Create(AutotuneTask, "Autotune", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);

		TaskMonitorAdd(TASKINFO_RUNNING_AUTOTUNE, taskHandle);
	}
	return 0;
}

MODULE_INITCALL(AutotuneInitialize, AutotuneStart)

static void at_new_actuators(UAVObjEvent * ev, void *ctx, void *obj, int len) {
	(void) ev; (void) ctx;

	static bool last_sample_unpushed = false;

	static bool running = false;
	static bool first_cycle = false;

	if (last_sample_unpushed) {
		/* Last time we were unable to advance the write pointer.
		 * Try again, last chance! */
		if (circ_queue_advance_write(at_queue)) {
			at_points_spilled++;
		}
	}

	ActuatorDesiredData actuators;
	ActuatorDesiredGet(&actuators);

	GyrosData g;
	GyrosGet(&g);

	struct at_queued_data *q_item = circ_queue_write_pos(at_queue,
			NULL, NULL);

	if (actuators.SystemIdentCycle == 0xffff) {
		if (running) {
			// Signify end of actuation stream.
			q_item->throttle = THROTTLE_EOF;

			if (circ_queue_advance_write(at_queue) == 0) {
				running = false;
				first_cycle = false;
			}
		}

		return;		// No data
	}

	/* Simple state machine.
	 * !running !first_cycle -> running first_cycle -> running !first_cycle
	 */
	if (!running || first_cycle) {
		if (actuators.SystemIdentCycle == 0x0000) {
			running = true;
			first_cycle = ! first_cycle;
		}
	}

#ifdef AUTOTUNE_AVERAGING_MODE
	struct at_measurement *avg_point = &at_averages[actuators.SystemIdentCycle];

	if (first_cycle) {
		*avg_point = (struct at_measurement) { { 0 } };
	}

	avg_point->y[0] += g.x;
	avg_point->y[1] += g.y;
	avg_point->y[2] += g.z;

	avg_point->u[0] += actuators.Roll;
	avg_point->u[1] += actuators.Pitch;
	avg_point->u[2] += actuators.Yaw;
#endif

	q_item->sample_num = actuators.SystemIdentCycle;

	q_item->meas.y[0] = g.x;
	q_item->meas.y[1] = g.y;
	q_item->meas.y[2] = g.z;

	q_item->meas.u[0] = actuators.Roll;
	q_item->meas.u[1] = actuators.Pitch;
	q_item->meas.u[2] = actuators.Yaw;

	q_item->throttle = actuators.Thrust;

	if (circ_queue_advance_write(at_queue) != 0) {
		last_sample_unpushed = true;
	} else {
		last_sample_unpushed = false;
	}
}

static void UpdateSystemIdent(const float *X, const float *noise,
		uint32_t predicts, uint32_t spills,
		float hover_throttle, bool new_tune) {
	SystemIdentData system_ident;

	system_ident.NewTune                        = new_tune;

	system_ident.Beta[SYSTEMIDENT_BETA_ROLL]    = X[6];
	system_ident.Beta[SYSTEMIDENT_BETA_PITCH]   = X[7];
	system_ident.Beta[SYSTEMIDENT_BETA_YAW]     = X[8];
	system_ident.Bias[SYSTEMIDENT_BIAS_ROLL]    = X[10];
	system_ident.Bias[SYSTEMIDENT_BIAS_PITCH]   = X[11];
	system_ident.Bias[SYSTEMIDENT_BIAS_YAW]     = X[12];
	system_ident.Tau                            = X[9];

	system_ident.Noise[SYSTEMIDENT_NOISE_ROLL]  = noise[0];
	system_ident.Noise[SYSTEMIDENT_NOISE_PITCH] = noise[1];
	system_ident.Noise[SYSTEMIDENT_NOISE_YAW]   = noise[2];

	system_ident.NumAfPredicts = predicts;
	system_ident.NumSpilledPts = spills;

	system_ident.HoverThrottle = hover_throttle;

	SystemIdentSet(&system_ident);
}

#ifdef AUTOTUNE_AVERAGING_MODE
static int autotune_save_averaging() {
	uintptr_t part_id;

	if (PIOS_FLASH_find_partition_id(
				FLASH_PARTITION_LABEL_AUTOTUNE,
				&part_id)) {
		return -1;
	}


	if (PIOS_FLASH_start_transaction(part_id)) {
		return -2;
	}

	if (PIOS_FLASH_erase_partition(part_id)) {
		PIOS_FLASH_end_transaction(part_id);
		return -3;
	}

	struct at_flash_header hdr = {
		.magic = ATFLASH_MAGIC,
		.wiggle_points = ident_wiggle_points,
		.aux_data_len = 0,
		.sample_rate = PIOS_SENSORS_GetSampleRate(PIOS_SENSOR_GYRO),
	};

	uint32_t offset = 0;

	if (PIOS_FLASH_write_data(part_id, 0, (uint8_t *) &hdr,
				sizeof(hdr))) {
		PIOS_FLASH_end_transaction(part_id);
		return -4;
	}

	offset += sizeof(hdr);

	if (PIOS_FLASH_write_data(part_id, offset, (uint8_t *) at_averages,
				sizeof(*at_averages) * ident_wiggle_points)) {
		PIOS_FLASH_end_transaction(part_id);
		return -5;
	}

	PIOS_FLASH_end_transaction(part_id);
	return 0;
}
#endif /* AUTOTUNE_AVERAGING_MODE */

/**
 * Module thread, should not return.
 */
static void AutotuneTask(void *parameters)
{
	enum AUTOTUNE_STATE state = AT_INIT;

	float X[AF_NUMX] = {0};
	float P[AF_NUMP] = {0};
	float noise[3] = {0};

	uint16_t last_samp = 0;

	const uint32_t YIELD_MS = 2;

#ifdef AUTOTUNE_AVERAGING_MODE
	/* Wait for stabilization module to complete startup and tell us
	 * its dT, etc.
	 */
	while (!ident_wiggle_points) {
		PIOS_Thread_Sleep(25);
	}

	uint16_t buf_size = sizeof(*at_averages) * ident_wiggle_points;
	at_averages = PIOS_malloc(buf_size);

	while (!at_averages) {
		/* Infinite loop because we couldn't get our buffer */
		/* Assert alarm XXX? */
		PIOS_Thread_Sleep(2500);
	}
#endif /* AUTOTUNE_AVERAGING_MODE */

	float dT_expected = 0.001f;
	uint16_t sample_rate = PIOS_SENSORS_GetSampleRate(PIOS_SENSOR_GYRO);

	if (sample_rate) {
		dT_expected = 1.0f / sample_rate;
	}

	ActuatorDesiredConnectCallback(at_new_actuators);

	bool save_needed = false;

	while(1) {
		static uint32_t update_counter = 0;

		uint8_t armed;

		FlightStatusArmedGet(&armed);

		if (save_needed) {
			if (armed == FLIGHTSTATUS_ARMED_DISARMED) {
#ifdef AUTOTUNE_AVERAGING_MODE
				if (autotune_save_averaging()) {
					// Try again in a second, I guess.
					PIOS_Thread_Sleep(1000);
					continue;
				}
#endif /* AUTOTUNE_AVERAGING_MODE */

				// Save the settings locally.
				UAVObjSave(SystemIdentHandle(), 0);
				state = AT_INIT;

				save_needed = false;
			}

		}

		// Only allow this module to run when roll axis is in a tune
		// mode
		uint8_t stab_modes[STABILIZATIONDESIRED_STABILIZATIONMODE_NUMELEM];

		StabilizationDesiredStabilizationModeGet(stab_modes);

		switch (stab_modes[0]) {
			case STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENT:
			case STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENTRATE:
				break;
			default:
				state = AT_INIT;
				PIOS_Thread_Sleep(30);
				continue;
		}

		switch(state) {
			case AT_INIT:
				// Reset save status; only save if this tune
				// completes.
				save_needed = false;

				// Only start when armed and flying
				if (armed == FLIGHTSTATUS_ARMED_ARMED) {

					af_init(X,P);

					state = AT_START;

				}

				break;

			case AT_START:
				/* Drain the queue of all current data */
				circ_queue_clear(at_queue);

				/* And reset the point spill counter */

				update_counter = 0;
				at_points_spilled = 0;

				throttle_accumulator = 0;

				state = AT_WAITFIRSTPOINT;

				break;

			case AT_WAITFIRSTPOINT:
				{
					struct at_queued_data *pt;

					/* Grab an autotune point */
					pt = circ_queue_read_pos(at_queue, NULL, NULL);

					if (!pt) {
						// Still no data showing up
						break;
					}

					if (pt->throttle == THROTTLE_EOF) {
						// Ignore old EOFs, just in case
						break;
					}

					state = AT_RUN;
				}

			case AT_RUN:
				while (true) {
					struct at_queued_data *pt;

					/* Grab an autotune point */
					pt = circ_queue_read_pos(at_queue, NULL, NULL);

					if (!pt) {
						/* We've drained the buffer
						 * fully.  Yay! */
						break;
					}

					if (pt->throttle == THROTTLE_EOF) {
						// End of actuation, clean up
						float hover_throttle = ((float)(throttle_accumulator/update_counter))/10000.0f;
						UpdateSystemIdent(X, noise, update_counter, at_points_spilled, hover_throttle, true);

						save_needed = true;
						state = AT_WAITING;

						break;
					}

					/* calculate time between successive
					 * points */

					uint16_t samp_interval = pt->sample_num - last_samp;

					if (samp_interval > ident_wiggle_points) {
						/* Handle wrap case. */
						samp_interval += ident_wiggle_points;
					}

					if ((samp_interval == 0) ||
							(samp_interval > 10)) {
						samp_interval = 10;
					}

					float dT_s = samp_interval * dT_expected;

					last_samp = pt->sample_num;

					af_predict(X, P, &pt->meas, dT_s, pt->throttle);

					for (uint32_t i = 0; i < 3; i++) {
						const float NOISE_ALPHA = 0.9997f;  // 10 second time constant at 300 Hz
						noise[i] = NOISE_ALPHA * noise[i] + (1-NOISE_ALPHA) * (pt->meas.y[i] - X[i]) * (pt->meas.y[i] - X[i]);
					}

					//This will work up to 8kHz with an 89% throttle position before overflow
					throttle_accumulator += 10000 * pt->throttle;

					/* Free the buffer containing an AT point */
					circ_queue_read_completed(at_queue);

					// Update uavo every 256 cycles to avoid
					// telemetry spam
					if (!((++update_counter) & 0xff)) {
						float hover_throttle = ((float)(throttle_accumulator/update_counter))/10000.0f;
						UpdateSystemIdent(X, noise, update_counter, at_points_spilled, hover_throttle, false);
					}
				}

				break;

			case AT_WAITING:
			default:
				break;
		}

		PIOS_Thread_Sleep(YIELD_MS);
	}
}

/**
 * Prediction step for EKF on control inputs to quad that
 * learns the system properties
 * @param X the current state estimate which is updated in place
 * @param P the current covariance matrix, updated in place
 * @param[in] the current control inputs (roll, pitch, yaw)
 * @param[in] the gyro measurements
 */
__attribute__((always_inline)) static inline void af_predict(float X[AF_NUMX], float P[AF_NUMP], const struct at_measurement *measurement, const float dT_s, const float t_in)
{
	const float Ts = dT_s;
	const float Tsq = Ts * Ts;
	const float Tsq3 = Tsq * Ts;
	const float Tsq4 = Tsq * Tsq;

	// for convenience and clarity code below uses the named versions of
	// the state variables
	float w1 = X[0];           // roll rate estimate
	float w2 = X[1];           // pitch rate estimate
	float w3 = X[2];           // yaw rate estimate
	float u1 = X[3];           // scaled roll torque
	float u2 = X[4];           // scaled pitch torque
	float u3 = X[5];           // scaled yaw torque
	const float e_b1 = expapprox(X[6]);   // roll torque scale
	const float b1 = X[6];
	const float e_b2 = expapprox(X[7]);   // pitch torque scale
	const float b2 = X[7];
	const float e_b3 = expapprox(X[8]);   // yaw torque scale
	const float b3 = X[8];
	const float e_tau = expapprox(X[9]); // time response of the motors
	const float tau = X[9];
	const float bias1 = X[10];        // bias in the roll torque
	const float bias2 = X[11];       // bias in the pitch torque
	const float bias3 = X[12];       // bias in the yaw torque

	// inputs to the system (roll, pitch, yaw)
	const float u1_in = 4*t_in*measurement->u[0];
	const float u2_in = 4*t_in*measurement->u[1];
	const float u3_in = 4*t_in*measurement->u[2];

	// measurements from gyro
	const float gyro_x = measurement->y[0];
	const float gyro_y = measurement->y[1];
	const float gyro_z = measurement->y[2];

	// update named variables because we want to use predicted
	// values below
	w1 = X[0] = w1 - Ts*bias1*e_b1 + Ts*u1*e_b1;
	w2 = X[1] = w2 - Ts*bias2*e_b2 + Ts*u2*e_b2;
	w3 = X[2] = w3 - Ts*bias3*e_b3 + Ts*u3*e_b3;
	u1 = X[3] = (Ts*u1_in)/(Ts + e_tau) + (u1*e_tau)/(Ts + e_tau);
	u2 = X[4] = (Ts*u2_in)/(Ts + e_tau) + (u2*e_tau)/(Ts + e_tau);
	u3 = X[5] = (Ts*u3_in)/(Ts + e_tau) + (u3*e_tau)/(Ts + e_tau);
    // X[6] to X[12] unchanged

	/**** filter parameters ****/
	const float q_w = 1e-3f;
	const float q_ud = 1e-3f;
	const float q_B = 1e-6f;
	const float q_tau = 1e-6f;
	const float q_bias = 1e-19f;
	const float s_a = 150.0f; // expected gyro measurement noise

	const float Q[AF_NUMX] = {q_w, q_w, q_w, q_ud, q_ud, q_ud, q_B, q_B, q_B, q_tau, q_bias, q_bias, q_bias};

	float D[AF_NUMP];
	for (uint32_t i = 0; i < AF_NUMP; i++)
        D[i] = P[i];

    const float e_tau2    = e_tau * e_tau;
    const float e_tau3    = e_tau * e_tau2;
    const float e_tau4    = e_tau2 * e_tau2;
    const float Ts_e_tau2 = (Ts + e_tau) * (Ts + e_tau);
    const float Ts_e_tau4 = Ts_e_tau2 * Ts_e_tau2;

	// covariance propagation - D is stored copy of covariance
	P[0] = D[0] + Q[0] + 2*Ts*e_b1*(D[3] - D[28] - D[9]*bias1 + D[9]*u1) + Tsq*(e_b1*e_b1)*(D[4] - 2*D[29] + D[32] - 2*D[10]*bias1 + 2*D[30]*bias1 + 2*D[10]*u1 - 2*D[30]*u1 + D[11]*(bias1*bias1) + D[11]*(u1*u1) - 2*D[11]*bias1*u1);
	P[1] = D[1] + Q[1] + 2*Ts*e_b2*(D[5] - D[33] - D[12]*bias2 + D[12]*u2) + Tsq*(e_b2*e_b2)*(D[6] - 2*D[34] + D[37] - 2*D[13]*bias2 + 2*D[35]*bias2 + 2*D[13]*u2 - 2*D[35]*u2 + D[14]*(bias2*bias2) + D[14]*(u2*u2) - 2*D[14]*bias2*u2);
	P[2] = D[2] + Q[2] + 2*Ts*e_b3*(D[7] - D[38] - D[15]*bias3 + D[15]*u3) + Tsq*(e_b3*e_b3)*(D[8] - 2*D[39] + D[42] - 2*D[16]*bias3 + 2*D[40]*bias3 + 2*D[16]*u3 - 2*D[40]*u3 + D[17]*(bias3*bias3) + D[17]*(u3*u3) - 2*D[17]*bias3*u3);
	P[3] = (D[3]*(e_tau2 + Ts*e_tau) + Ts*e_b1*e_tau2*(D[4] - D[29]) + Tsq*e_b1*e_tau*(D[4] - D[29]) + D[18]*Ts*e_tau*(u1 - u1_in) + D[10]*e_b1*(u1*(Ts*e_tau2 + Tsq*e_tau) - bias1*(Ts*e_tau2 + Tsq*e_tau)) + D[21]*Tsq*e_b1*e_tau*(u1 - u1_in) + D[31]*Tsq*e_b1*e_tau*(u1_in - u1) + D[24]*Tsq*e_b1*e_tau*(u1*(u1 - bias1) + u1_in*(bias1 - u1)))/Ts_e_tau2;
	P[4] = (Q[3]*Tsq4 + e_tau4*(D[4] + Q[3]) + 2*Ts*e_tau3*(D[4] + 2*Q[3]) + 4*Q[3]*Tsq3*e_tau + Tsq*e_tau2*(D[4] + 6*Q[3] + u1*(D[27]*u1 + 2*D[21]) + u1_in*(D[27]*u1_in - 2*D[21])) + 2*D[21]*Ts*e_tau3*(u1 - u1_in) - 2*D[27]*Tsq*u1*u1_in*e_tau2)/Ts_e_tau4;
	P[5] = (D[5]*(e_tau2 + Ts*e_tau) + Ts*e_b2*e_tau2*(D[6] - D[34]) + Tsq*e_b2*e_tau*(D[6] - D[34]) + D[19]*Ts*e_tau*(u2 - u2_in) + D[13]*e_b2*(u2*(Ts*e_tau2 + Tsq*e_tau) - bias2*(Ts*e_tau2 + Tsq*e_tau)) + D[22]*Tsq*e_b2*e_tau*(u2 - u2_in) + D[36]*Tsq*e_b2*e_tau*(u2_in - u2) + D[25]*Tsq*e_b2*e_tau*(u2*(u2 - bias2) + u2_in*(bias2 - u2)))/Ts_e_tau2;
	P[6] = (Q[4]*Tsq4 + e_tau4*(D[6] + Q[4]) + 2*Ts*e_tau3*(D[6] + 2*Q[4]) + 4*Q[4]*Tsq3*e_tau + Tsq*e_tau2*(D[6] + 6*Q[4] + u2*(D[27]*u2 + 2*D[22]) + u2_in*(D[27]*u2_in - 2*D[22])) + 2*D[22]*Ts*e_tau3*(u2 - u2_in) - 2*D[27]*Tsq*u2*u2_in*e_tau2)/Ts_e_tau4;
	P[7] = (D[7]*(e_tau2 + Ts*e_tau) + Ts*e_b3*e_tau2*(D[8] - D[39]) + Tsq*e_b3*e_tau*(D[8] - D[39]) + D[20]*Ts*e_tau*(u3 - u3_in) + D[16]*e_b3*(u3*(Ts*e_tau2 + Tsq*e_tau) - bias3*(Ts*e_tau2 + Tsq*e_tau)) + D[23]*Tsq*e_b3*e_tau*(u3 - u3_in) + D[41]*Tsq*e_b3*e_tau*(u3_in - u3) + D[26]*Tsq*e_b3*e_tau*(u3*(u3 - bias3) + u3_in*(bias3 - u3)))/Ts_e_tau2;
	P[8] = (Q[5]*Tsq4 + e_tau4*(D[8] + Q[5]) + 2*Ts*e_tau3*(D[8] + 2*Q[5]) + 4*Q[5]*Tsq3*e_tau + Tsq*e_tau2*(D[8] + 6*Q[5] + u3*(D[27]*u3 + 2*D[23]) + u3_in*(D[27]*u3_in - 2*D[23])) + 2*D[23]*Ts*e_tau3*(u3 - u3_in) - 2*D[27]*Tsq*u3*u3_in*e_tau2)/Ts_e_tau4;
	P[9] = D[9] - Ts*e_b1*(D[30] - D[10] + D[11]*(bias1 - u1));
	P[10] = (D[10]*(Ts + e_tau) + D[24]*Ts*(u1 - u1_in))*(e_tau/Ts_e_tau2);
	P[11] = D[11] + Q[6];
	P[12] = D[12] - Ts*e_b2*(D[35] - D[13] + D[14]*(bias2 - u2));
	P[13] = (D[13]*(Ts + e_tau) + D[25]*Ts*(u2 - u2_in))*(e_tau/Ts_e_tau2);
	P[14] = D[14] + Q[7];
	P[15] = D[15] - Ts*e_b3*(D[40] - D[16] + D[17]*(bias3 - u3));
	P[16] = (D[16]*(Ts + e_tau) + D[26]*Ts*(u3 - u3_in))*(e_tau/Ts_e_tau2);
	P[17] = D[17] + Q[8];
	P[18] = D[18] - Ts*e_b1*(D[31] - D[21] + D[24]*(bias1 - u1));
	P[19] = D[19] - Ts*e_b2*(D[36] - D[22] + D[25]*(bias2 - u2));
	P[20] = D[20] - Ts*e_b3*(D[41] - D[23] + D[26]*(bias3 - u3));
	P[21] = (D[21]*(Ts + e_tau) + D[27]*Ts*(u1 - u1_in))*(e_tau/Ts_e_tau2);
	P[22] = (D[22]*(Ts + e_tau) + D[27]*Ts*(u2 - u2_in))*(e_tau/Ts_e_tau2);
	P[23] = (D[23]*(Ts + e_tau) + D[27]*Ts*(u3 - u3_in))*(e_tau/Ts_e_tau2);
	P[24] = D[24];
	P[25] = D[25];
	P[26] = D[26];
	P[27] = D[27] + Q[9];
	P[28] = D[28] - Ts*e_b1*(D[32] - D[29] + D[30]*(bias1 - u1));
	P[29] = (D[29]*(Ts + e_tau) + D[31]*Ts*(u1 - u1_in))*(e_tau/Ts_e_tau2);
	P[30] = D[30];
	P[31] = D[31];
	P[32] = D[32] + Q[10];
	P[33] = D[33] - Ts*e_b2*(D[37] - D[34] + D[35]*(bias2 - u2));
	P[34] = (D[34]*(Ts + e_tau) + D[36]*Ts*(u2 - u2_in))*(e_tau/Ts_e_tau2);
	P[35] = D[35];
	P[36] = D[36];
	P[37] = D[37] + Q[11];
	P[38] = D[38] - Ts*e_b3*(D[42] - D[39] + D[40]*(bias3 - u3));
	P[39] = (D[39]*(Ts + e_tau) + D[41]*Ts*(u3 - u3_in))*(e_tau/Ts_e_tau2);
	P[40] = D[40];
	P[41] = D[41];
	P[42] = D[42] + Q[12];


	/********* this is the update part of the equation ***********/

    float S[3] = {P[0] + s_a, P[1] + s_a, P[2] + s_a};

	X[0] = w1 + P[0]*((gyro_x - w1)/S[0]);
	X[1] = w2 + P[1]*((gyro_y - w2)/S[1]);
	X[2] = w3 + P[2]*((gyro_z - w3)/S[2]);
	X[3] = u1 + P[3]*((gyro_x - w1)/S[0]);
	X[4] = u2 + P[5]*((gyro_y - w2)/S[1]);
	X[5] = u3 + P[7]*((gyro_z - w3)/S[2]);
	X[6] = b1 + P[9]*((gyro_x - w1)/S[0]);
	X[7] = b2 + P[12]*((gyro_y - w2)/S[1]);
	X[8] = b3 + P[15]*((gyro_z - w3)/S[2]);
	X[9] = tau + P[18]*((gyro_x - w1)/S[0]) + P[19]*((gyro_y - w2)/S[1]) + P[20]*((gyro_z - w3)/S[2]);
	X[10] = bias1 + P[28]*((gyro_x - w1)/S[0]);
	X[11] = bias2 + P[33]*((gyro_y - w2)/S[1]);
	X[12] = bias3 + P[38]*((gyro_z - w3)/S[2]);

	// update the duplicate cache
	for (uint32_t i = 0; i < AF_NUMP; i++)
        D[i] = P[i];

	// This is an approximation that removes some cross axis uncertainty but
	// substantially reduces the number of calculations
	P[0] = -D[0]*(D[0]/S[0] - 1);
	P[1] = -D[1]*(D[1]/S[1] - 1);
	P[2] = -D[2]*(D[2]/S[2] - 1);
	P[3] = -D[3]*(D[0]/S[0] - 1);
	P[4] = D[4] - D[3]*D[3]/S[0];
	P[5] = -D[5]*(D[1]/S[1] - 1);
	P[6] = D[6] - D[5]*D[5]/S[1];
	P[7] = -D[7]*(D[2]/S[2] - 1);
	P[8] = D[8] - D[7]*D[7]/S[2];
	P[9] = -D[9]*(D[0]/S[0] - 1);
	P[10] = D[10] - D[3]*(D[9]/S[0]);
	P[11] = D[11] - D[9]*(D[9]/S[0]);
	P[12] = -D[12]*(D[1]/S[1] - 1);
	P[13] = D[13] - D[5]*(D[12]/S[1]);
	P[14] = D[14] - D[12]*(D[12]/S[1]);
	P[15] = -D[15]*(D[2]/S[2] - 1);
	P[16] = D[16] - D[7]*(D[15]/S[2]);
	P[17] = D[17] - D[15]*(D[15]/S[2]);
	P[18] = -D[18]*(D[0]/S[0] - 1);
	P[19] = -D[19]*(D[1]/S[1] - 1);
	P[20] = -D[20]*(D[2]/S[2] - 1);
	P[21] = D[21] - D[3]*(D[18]/S[0]);
	P[22] = D[22] - D[5]*(D[19]/S[1]);
	P[23] = D[23] - D[7]*(D[20]/S[2]);
	P[24] = D[24] - D[9]*(D[18]/S[0]);
	P[25] = D[25] - D[12]*(D[19]/S[1]);
	P[26] = D[26] - D[15]*(D[20]/S[2]);
	P[27] = D[27] - D[18]*(D[18]/S[0]) - D[19]*(D[19]/S[1]) - D[20]*(D[20]/S[2]);
	P[28] = -D[28]*(D[0]/S[0] - 1);
	P[29] = D[29] - D[3]*(D[28]/S[0]);
	P[30] = D[30] - D[9]*(D[28]/S[0]);
	P[31] = D[31] - D[18]*(D[28]/S[0]);
	P[32] = D[32] - D[28]*(D[28]/S[0]);
	P[33] = -D[33]*(D[1]/S[1] - 1);
	P[34] = D[34] - D[5]*(D[33]/S[1]);
	P[35] = D[35] - D[12]*(D[33]/S[1]);
	P[36] = D[36] - D[19]*(D[33]/S[1]);
	P[37] = D[37] - D[33]*(D[33]/S[1]);
	P[38] = -D[38]*(D[2]/S[2] - 1);
	P[39] = D[39] - D[7]*(D[38]/S[2]);
	P[40] = D[40] - D[15]*(D[38]/S[2]);
	P[41] = D[41] - D[20]*(D[38]/S[2]);
	P[42] = D[42] - D[38]*(D[38]/S[2]);

	// apply limits to some of the state variables
	if (X[9] > -1.5f)
	    X[9] = -1.5f;
	if (X[9] < -5.5f)		/* 4ms */
	    X[9] = -5.5f;
	if (X[10] > 0.5f)
	    X[10] = 0.5f;
	if (X[10] < -0.5f)
	    X[10] = -0.5f;
	if (X[11] > 0.5f)
	    X[11] = 0.5f;
	if (X[11] < -0.5f)
	    X[11] = -0.5f;
	if (X[12] > 0.5f)
	    X[12] = 0.5f;
	if (X[12] < -0.5f)
	    X[12] = -0.5f;
}

/**
 * Initialize the state variable and covariance matrix
 * for the system identification EKF
 */
static void af_init(float X[AF_NUMX], float P[AF_NUMP])
{
	const float q_init[AF_NUMX] = {
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		0.05f, 0.05f, 0.005f,
		0.05f,
		0.05f, 0.05f, 0.05f
	};

	X[0] = X[1] = X[2] = 0.0f;    // assume no rotation
	X[3] = X[4] = X[5] = 0.0f;    // and no net torque
	X[6] = X[7]        = 10.0f;   // medium amount of strength
	X[8]               = 7.0f;    // yaw
	X[9] = -4.0f;                 // and 50 ms time scale
	X[10] = X[11] = X[12] = 0.0f; // zero bias

	// P initialization
	// Could zero this like: *P = *((float [AF_NUMP]){});
	P[0] = q_init[0];
	P[1] = q_init[1];
	P[2] = q_init[2];
	P[3] = 0.0f;
	P[4] = q_init[3];
	P[5] = 0.0f;
	P[6] = q_init[4];
	P[7] = 0.0f;
	P[8] = q_init[5];
	P[9] = 0.0f;
	P[10] = 0.0f;
	P[11] = q_init[6];
	P[12] = 0.0f;
	P[13] = 0.0f;
	P[14] = q_init[7];
	P[15] = 0.0f;
	P[16] = 0.0f;
	P[17] = q_init[8];
	P[18] = 0.0f;
	P[19] = 0.0f;
	P[20] = 0.0f;
	P[21] = 0.0f;
	P[22] = 0.0f;
	P[23] = 0.0f;
	P[24] = 0.0f;
	P[25] = 0.0f;
	P[26] = 0.0f;
	P[27] = q_init[9];
	P[28] = 0.0f;
	P[29] = 0.0f;
	P[30] = 0.0f;
	P[31] = 0.0f;
	P[32] = q_init[10];
	P[33] = 0.0f;
	P[34] = 0.0f;
	P[35] = 0.0f;
	P[36] = 0.0f;
	P[37] = q_init[11];
	P[38] = 0.0f;
	P[39] = 0.0f;
	P[40] = 0.0f;
	P[41] = 0.0f;
	P[42] = q_init[12];
}

/**
 * @}
 * @}
 */
