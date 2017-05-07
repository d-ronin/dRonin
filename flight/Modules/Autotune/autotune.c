/**
 ******************************************************************************
 * @addtogroup Modules Modules
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

#include "misc_math.h"

// Private constants
#define STACK_SIZE_BYTES 640
#define TASK_PRIORITY PIOS_THREAD_PRIO_NORMAL

#ifndef AUTOTUNE_AVERAGING_DECIMATION
#define AUTOTUNE_AVERAGING_DECIMATION 1
#endif

// Private types
enum autotune_state { AT_INIT, AT_RUN };

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

static struct at_measurement *at_averages;

// Private variables
static struct pios_thread *taskHandle;
static bool module_enabled;

static volatile uint32_t throttle_accumulator;
static volatile uint32_t update_counter = 0;
static volatile bool tune_running = false;
extern uint16_t ident_wiggle_points;

uint16_t decim_wiggle_points;

// Private functions
static void AutotuneTask(void *parameters);

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AutotuneInitialize(void)
{
	if (SystemIdentInitialize() == -1) {
		module_enabled = false;
		return -1;
	}

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

	static bool first_cycle = false;

	ActuatorDesiredData actuators;
	ActuatorDesiredGet(&actuators);

	GyrosData g;
	GyrosGet(&g);

	if (actuators.SystemIdentCycle == 0xffff) {
		if (tune_running) {
			tune_running = false;
			first_cycle = false;
		}

		return;		// No data
	}

	/* Simple state machine.
	 * !running !first_cycle -> running first_cycle -> running !first_cycle
	 */
	if (!tune_running || first_cycle) {
		if (actuators.SystemIdentCycle == 0x0000) {
			if (!tune_running) {
				update_counter = 0;
				throttle_accumulator = 0;
			}

			tune_running = true;
			first_cycle = ! first_cycle;
		}
	}

	struct at_measurement *avg_point = &at_averages[actuators.SystemIdentCycle / AUTOTUNE_AVERAGING_DECIMATION];

	if (first_cycle) {
		*avg_point = (struct at_measurement) { { 0 } };
	}

	avg_point->y[0] += g.x;
	avg_point->y[1] += g.y;
	avg_point->y[2] += g.z;

	avg_point->u[0] += actuators.Roll;
	avg_point->u[1] += actuators.Pitch;
	avg_point->u[2] += actuators.Yaw;

	update_counter++;
	throttle_accumulator += 10000 * actuators.Thrust;
}

static void UpdateSystemIdent(uint32_t predicts, float hover_throttle,
		bool new_tune) {
	SystemIdentData system_ident;

	system_ident.NewTune = new_tune;
	system_ident.NumAfPredicts = predicts;

	system_ident.HoverThrottle = hover_throttle;

	SystemIdentSet(&system_ident);
}

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
		.wiggle_points = decim_wiggle_points,
		.aux_data_len = 0,
		.sample_rate = PIOS_Sensors_GetUpdateRate(PIOS_Sensors_GetSensor(PIOS_SENSOR_GYRO)) / AUTOTUNE_AVERAGING_DECIMATION,
	};

	uint32_t offset = 0;

	if (PIOS_FLASH_write_data(part_id, 0, (uint8_t *) &hdr,
				sizeof(hdr))) {
		PIOS_FLASH_end_transaction(part_id);
		return -4;
	}

	offset += sizeof(hdr);

	if (PIOS_FLASH_write_data(part_id, offset, (uint8_t *) at_averages,
				sizeof(*at_averages) * decim_wiggle_points)) {
		PIOS_FLASH_end_transaction(part_id);
		return -5;
	}

	PIOS_FLASH_end_transaction(part_id);
	return 0;
}

/**
 * Module thread, should not return.
 */
static void AutotuneTask(void *parameters)
{
	const uint32_t YIELD_MS = 100;

	/* Wait for stabilization module to complete startup and tell us
	 * its dT, etc.
	 */
	while (!ident_wiggle_points) {
		PIOS_Thread_Sleep(25);
	}

	decim_wiggle_points = ident_wiggle_points / AUTOTUNE_AVERAGING_DECIMATION;

	uint16_t buf_size = sizeof(*at_averages) * decim_wiggle_points;
	at_averages = PIOS_malloc(buf_size);

	while (!at_averages) {
		/* Infinite loop because we couldn't get our buffer */
		/* Assert alarm XXX? */
		PIOS_Thread_Sleep(2500);
	}

	ActuatorDesiredConnectCallback(at_new_actuators);

	bool save_needed = false;
	enum autotune_state state = AT_INIT;

	while(1) {
		uint8_t armed;

		FlightStatusArmedGet(&armed);

		if (save_needed) {
			if (armed == FLIGHTSTATUS_ARMED_DISARMED) {
				if (autotune_save_averaging()) {
					// Try again in a second, I guess.
					PIOS_Thread_Sleep(1000);
					continue;
				}

				float hover_throttle = ((float) (throttle_accumulator / update_counter)) / 10000.0f;

				UpdateSystemIdent(update_counter, hover_throttle,
						true);

				// Save the UAVO locally.
				UAVObjSave(SystemIdentHandle(), 0);
				state = AT_INIT;

				save_needed = false;
			}

		}

		switch(state) {
			default:
			case AT_INIT:
				if (tune_running) {
					// Reset save status; only save if this
					// tune completes.
					save_needed = false;
					state = AT_RUN;
				}

				break;

			case AT_RUN:
				(void) 0;

				float hover_throttle = ((float) (throttle_accumulator / update_counter)) / 10000.0f;

				UpdateSystemIdent(update_counter, hover_throttle,
						false);

				if (!tune_running) {
					/* Threshold: 24 seconds of data @ 500Hz */
					if (update_counter > 12000) {
						save_needed = true;
					}

					state = AT_INIT;
				}

				break;
		}

		PIOS_Thread_Sleep(YIELD_MS);
	}
}

/**
 * @}
 * @}
 */
