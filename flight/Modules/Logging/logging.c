/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{ 
 * @addtogroup Logging Logging Module
 * @{ 
 *
 * @file       logging.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @brief      Forward a set of UAVObjects when updated out a PIOS_COM port
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "openpilot.h"
#include "modulesettings.h"
#include "pios_thread.h"
#include "pios_queue.h"
#include "pios_mutex.h"
#include "uavobjectmanager.h"
#include "misc_math.h"
#include "timeutils.h"
#include "uavobjectmanager.h"

#include "pios_streamfs.h"
#include <pios_board_info.h>

#include "accels.h"
#include "actuatorcommand.h"
#include "actuatordesired.h"
#include "airspeedactual.h"
#include "attitudeactual.h"
#include "baroaltitude.h"
#include "flightbatterystate.h"
#include "flightstatus.h"
#include "gpsposition.h"
#include "gpstime.h"
#include "gpssatellites.h"
#include "gyros.h"
#include "loggingsettings.h"
#include "loggingstats.h"
#include "magnetometer.h"
#include "manualcontrolcommand.h"
#include "positionactual.h"
#include "stabilizationdesired.h"
#include "systemalarms.h"
#include "systemident.h"
#include "velocityactual.h"
#include "waypointactive.h"

#include "pios_bl_helper.h"
#include "pios_streamfs_priv.h"

#include "pios_com_priv.h"

#include <uavtalk.h>

// Private constants
#define STACK_SIZE_BYTES 1200
#define TASK_PRIORITY PIOS_THREAD_PRIO_LOW
const char DIGITS[16] = "0123456789abcdef";

#define LOGGING_PERIOD_MS 100

// Private types

// Private variables
static UAVTalkConnection uavTalkCon;
static struct pios_thread *loggingTaskHandle;
static bool module_enabled;
static volatile LoggingSettingsData settings;
static LoggingStatsData loggingData;

// Private functions
static void    loggingTask(void *parameters);
static int32_t send_data(uint8_t *data, int32_t length);
static int32_t send_data_nonblock(void *ctx, uint8_t *data, int32_t length);
static uint16_t get_minimum_logging_period();
static void unregister_object(UAVObjHandle obj);
static void register_object(UAVObjHandle obj);
static void register_default_profile();
static void logAll(UAVObjHandle obj);
static void logSettings(UAVObjHandle obj);
static void writeHeader();
static void updateSettings();

// Local variables
static uintptr_t logging_com_id;
static uint32_t written_bytes;
static bool destination_onboard_flash;

#ifdef PIOS_INCLUDE_LOG_TO_FLASH
static const struct streamfs_cfg streamfs_settings = {
	.fs_magic      = 0x89abceef,
	.arena_size    = PIOS_LOGFLASH_SECT_SIZE,
	.write_size    = 0x00000100, /* 256 bytes */
};
#endif

/**
 * Initialise the Logging module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t LoggingInitialize(void)
{
	if (LoggingSettingsInitialize() == -1) {
		module_enabled = false;
		return -1;
	}

#ifdef PIOS_COM_OPENLOG
	if (PIOS_COM_OPENLOG) {
		logging_com_id = PIOS_COM_OPENLOG;
		updateSettings();
	}
#endif

#ifdef PIOS_INCLUDE_LOG_TO_FLASH
	if (!logging_com_id) {
		uintptr_t streamfs_id;

		if (PIOS_STREAMFS_Init(&streamfs_id, &streamfs_settings, FLASH_PARTITION_LABEL_LOG) != 0) {
			module_enabled = false;
			return -1;
		}

		const uint32_t LOG_BUF_LEN = 768;
		if (PIOS_COM_Init(&logging_com_id, &pios_streamfs_com_driver,
				streamfs_id, 0, LOG_BUF_LEN) != 0) {
			module_enabled = false;
			return -1;
		}

		destination_onboard_flash = true;
		updateSettings();
	}
#endif
		
#ifdef MODULE_Logging_BUILTIN
	module_enabled = true;
#else
	uint8_t module_state[MODULESETTINGS_ADMINSTATE_NUMELEM];
	ModuleSettingsAdminStateGet(module_state);
	if (module_state[MODULESETTINGS_ADMINSTATE_LOGGING] == MODULESETTINGS_ADMINSTATE_ENABLED) {
		module_enabled = true;
	} else {
		module_enabled = false;
	}
#endif

	if (!logging_com_id) {
		module_enabled = false;
	}

	if (!module_enabled)
		return -1;

	if (LoggingStatsInitialize() == -1) {
		module_enabled = false;
		return -1;
	}

	// Initialise UAVTalk
	uavTalkCon = UAVTalkInitialize(NULL, &send_data_nonblock, NULL, NULL);

	if (!uavTalkCon) {
		module_enabled = false;
		return -1;
	}
	
	return 0;
}

/**
 * Start the Logging module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t LoggingStart(void)
{
	//Check if module is enabled or not
	if (module_enabled == false) {
		return -1;
	}

	// Start logging task
	loggingTaskHandle = PIOS_Thread_Create(loggingTask, "Logging", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);

	TaskMonitorAdd(TASKINFO_RUNNING_LOGGING, loggingTaskHandle);
	
	return 0;
}

MODULE_INITCALL(LoggingInitialize, LoggingStart);

static void loggingTask(void *parameters)
{
	bool armed = false;
	uint32_t now = PIOS_Thread_Systime();

#ifdef PIOS_INCLUDE_LOG_TO_FLASH
	bool write_open = false;
	bool read_open = false;
	int32_t read_sector = 0;
	uint8_t read_data[LOGGINGSTATS_FILESECTOR_NUMELEM];
#endif

	// Get settings automatically for now on
	LoggingSettingsConnectCopy(&settings);

	LoggingStatsGet(&loggingData);
	loggingData.BytesLogged = 0;
	
#ifdef PIOS_INCLUDE_LOG_TO_FLASH
	if (destination_onboard_flash) {
		loggingData.MinFileId = PIOS_STREAMFS_MinFileId(logging_com_id);
		loggingData.MaxFileId = PIOS_STREAMFS_MaxFileId(logging_com_id);
	}
#endif

	if (!destination_onboard_flash) {
		updateSettings();
	}

	if (settings.LogBehavior == LOGGINGSETTINGS_LOGBEHAVIOR_LOGONSTART) {
		loggingData.Operation = LOGGINGSTATS_OPERATION_INITIALIZING;
	} else {
		loggingData.Operation = LOGGINGSTATS_OPERATION_IDLE;
	}

	LoggingStatsSet(&loggingData);

	// Loop forever
	while (1) 
	{
		LoggingStatsGet(&loggingData);

		// Check for change in armed state if logging on armed

		if (settings.LogBehavior == LOGGINGSETTINGS_LOGBEHAVIOR_LOGONARM) {
			FlightStatusData flightStatus;
			FlightStatusGet(&flightStatus);

			if (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED && !armed) {
				// Start logging because just armed
				loggingData.Operation = LOGGINGSTATS_OPERATION_INITIALIZING;
				armed = true;
				LoggingStatsSet(&loggingData);
			} else if (flightStatus.Armed == FLIGHTSTATUS_ARMED_DISARMED && armed) {
				loggingData.Operation = LOGGINGSTATS_OPERATION_IDLE;
				armed = false;
				LoggingStatsSet(&loggingData);
			}
		}

		switch (loggingData.Operation) {
		case LOGGINGSTATS_OPERATION_FORMAT:
			// Format the file system
#ifdef PIOS_INCLUDE_LOG_TO_FLASH
			if (destination_onboard_flash){
				if (read_open || write_open) {
					PIOS_STREAMFS_Close(logging_com_id);
					read_open = false;
					write_open = false;
				}

				PIOS_STREAMFS_Format(logging_com_id);
				loggingData.MinFileId = PIOS_STREAMFS_MinFileId(logging_com_id);
				loggingData.MaxFileId = PIOS_STREAMFS_MaxFileId(logging_com_id);
			}
#endif /* PIOS_INCLUDE_LOG_TO_FLASH */
			loggingData.Operation = LOGGINGSTATS_OPERATION_IDLE;
			LoggingStatsSet(&loggingData);
			break;
		case LOGGINGSTATS_OPERATION_INITIALIZING:
			// Unregister all objects
			UAVObjIterate(&unregister_object);
#ifdef PIOS_INCLUDE_LOG_TO_FLASH
			if (destination_onboard_flash){
				// Close the file if it is open for reading
				if (read_open) {
					PIOS_STREAMFS_Close(logging_com_id);
					read_open = false;
				}

				// Open the file if it is not open for writing
				if (!write_open) {
					if (PIOS_STREAMFS_OpenWrite(logging_com_id) != 0) {
						loggingData.Operation = LOGGINGSTATS_OPERATION_ERROR;
						continue;
					} else {
						write_open = true;
					}
					loggingData.MinFileId = PIOS_STREAMFS_MinFileId(logging_com_id);
					loggingData.MaxFileId = PIOS_STREAMFS_MaxFileId(logging_com_id);
					LoggingStatsSet(&loggingData);
				}
			}
			else {
				read_open = false;
				write_open = true;
			}
#endif /* PIOS_INCLUDE_LOG_TO_FLASH */

			// Write information at start of the log file
			writeHeader();

			// Log settings
			if (settings.InitiallyLog == LOGGINGSETTINGS_INITIALLYLOG_ALLOBJECTS) {
				UAVObjIterate(&logAll);
			} else if (settings.InitiallyLog == LOGGINGSETTINGS_INITIALLYLOG_SETTINGSOBJECTS) {
				UAVObjIterate(&logSettings);
			}

			// Register objects to be logged
			switch (settings.Profile) {
				case LOGGINGSETTINGS_PROFILE_BASIC:
					register_default_profile();
					break;
				case LOGGINGSETTINGS_PROFILE_CUSTOM:
				case LOGGINGSETTINGS_PROFILE_FULLBORE:
					UAVObjIterate(&register_object);
					break;
			}

			// Empty the queue
			LoggingStatsBytesLoggedSet(&written_bytes);
			loggingData.Operation = LOGGINGSTATS_OPERATION_LOGGING;
			LoggingStatsSet(&loggingData);
			break;
		case LOGGINGSTATS_OPERATION_LOGGING:
			{
				// Sleep between updating stats.
				PIOS_Thread_Sleep_Until(&now, LOGGING_PERIOD_MS);

				LoggingStatsBytesLoggedSet(&written_bytes);

				now = PIOS_Thread_Systime();
			}
			break;
		case LOGGINGSTATS_OPERATION_DOWNLOAD:
#ifdef PIOS_INCLUDE_LOG_TO_FLASH
			if (destination_onboard_flash) {
				if (!read_open) {
					// Start reading
					if (PIOS_STREAMFS_OpenRead(logging_com_id, loggingData.FileRequest) != 0) {
						loggingData.Operation = LOGGINGSTATS_OPERATION_ERROR;
					} else {
						read_open = true;
						read_sector = -1;
					}
				}
				if (read_open && read_sector == loggingData.FileSectorNum) {
					// Request received for same sector. Reupdate.
					memcpy(loggingData.FileSector, read_data, LOGGINGSTATS_FILESECTOR_NUMELEM);
					loggingData.Operation = LOGGINGSTATS_OPERATION_IDLE;

				} else if (read_open && (read_sector + 1) == loggingData.FileSectorNum) {
					int32_t bytes_read = PIOS_STREAMFS_Read(logging_com_id, loggingData.FileSector, LOGGINGSTATS_FILESECTOR_NUMELEM);
					if (bytes_read < 0) {
						// close on error
						loggingData.Operation = LOGGINGSTATS_OPERATION_ERROR;
						loggingData.FileSectorNum = 0xffff;
						PIOS_STREAMFS_Close(logging_com_id);
						read_open = false;
					} else {
						// Indicate sent
						loggingData.Operation = LOGGINGSTATS_OPERATION_IDLE;

						if (bytes_read < LOGGINGSTATS_FILESECTOR_NUMELEM) {
							// indicate end of file
							loggingData.Operation = LOGGINGSTATS_OPERATION_COMPLETE;
							PIOS_STREAMFS_Close(logging_com_id);
							read_open = false;
						}

					}
				}
				LoggingStatsSet(&loggingData);

				// Store the data in case it's needed again /
				// lost over telemetry link
				memcpy(read_data, loggingData.FileSector, LOGGINGSTATS_FILESECTOR_NUMELEM);
				read_sector = loggingData.FileSectorNum;
			}
#endif /* PIOS_INCLUDE_LOG_TO_FLASH */

			// fall-through to default case
		default:
			//  Makes sure that we are not hogging the processor
			PIOS_Thread_Sleep(10);
#ifdef PIOS_INCLUDE_LOG_TO_FLASH
			if (destination_onboard_flash) {
				// Close the file if necessary
				if (write_open) {
					PIOS_STREAMFS_Close(logging_com_id);
					loggingData.MinFileId = PIOS_STREAMFS_MinFileId(logging_com_id);
					loggingData.MaxFileId = PIOS_STREAMFS_MaxFileId(logging_com_id);
					LoggingStatsSet(&loggingData);
					write_open = false;
				}
			}
#endif /* PIOS_INCLUDE_LOG_TO_FLASH */
		}
	}
}

/**
 * Log all objects' initial value.
 * \param[in] obj Object to log
*/
static void logAll(UAVObjHandle obj)
{
	UAVTalkSendObjectTimestamped(uavTalkCon, obj, 0);
}

 /**
  * Log all settings objects
  * \param[in] obj Object to log
  */
static void logSettings(UAVObjHandle obj)
{
	if (UAVObjIsSettings(obj)) {
		UAVTalkSendObjectTimestamped(uavTalkCon, obj, 0);
	}
}


static int32_t send_data(uint8_t *data, int32_t length)
{
	if (PIOS_COM_SendBuffer(logging_com_id, data, length) < 0)
		return -1;

	written_bytes += length;

	return length;
}

/**
 * Forward data from UAVTalk out the serial port
 * \param[in] data Data buffer to send
 * \param[in] length Length of buffer
 * \return -1 on failure
 * \return number of bytes transmitted on success
 */
static int32_t send_data_nonblock(void *ctx, uint8_t *data, int32_t length)
{
	(void) ctx;

	if (PIOS_COM_SendBufferNonBlocking(logging_com_id, data, length) < 0)
		return -1;

	written_bytes += length;

	return length;
}

/**
 * @brief Callback for adding an object to the logging queue
 * @param ev the event
 */
static void obj_updated_callback(UAVObjEvent * ev, void* cb_ctx, void *uavo_data, int uavo_len)
{
	(void) cb_ctx; (void) uavo_data; (void) uavo_len;

	if (loggingData.Operation != LOGGINGSTATS_OPERATION_LOGGING){
		// We are not logging, so all events are discarded
		return;
	}

	UAVTalkSendObjectTimestamped(uavTalkCon, ev->obj, ev->instId);
}


/**
 * Get the minimum logging period in milliseconds
*/
static uint16_t get_minimum_logging_period()
{
	uint16_t min_period = 200;

	switch (settings.MaxLogRate) {
		case LOGGINGSETTINGS_MAXLOGRATE_5:
			min_period = 200;
			break;
		case LOGGINGSETTINGS_MAXLOGRATE_10:
			min_period = 100;
			break;
		case LOGGINGSETTINGS_MAXLOGRATE_25:
			min_period = 40;
			break;
		case LOGGINGSETTINGS_MAXLOGRATE_50:
			min_period = 20;
			break;
		case LOGGINGSETTINGS_MAXLOGRATE_100:
			min_period = 10;
			break;
		case LOGGINGSETTINGS_MAXLOGRATE_166:
			min_period = 6;
			break;
		case LOGGINGSETTINGS_MAXLOGRATE_250:
			min_period = 4;
			break;
		case LOGGINGSETTINGS_MAXLOGRATE_333:
			min_period = 3;
			break;
		case LOGGINGSETTINGS_MAXLOGRATE_500:
			min_period = 2;
			break;
		case LOGGINGSETTINGS_MAXLOGRATE_1000:
			min_period = 1;
			break;
	}
	return min_period;
}


/**
 * Unregister an object
 * \param[in] obj Object to unregister
 */
static void unregister_object(UAVObjHandle obj) {
	UAVObjDisconnectCallback(obj, obj_updated_callback, NULL);
}

/**
 * Register a new object: connect the update callback
 * \param[in] obj Object to connect
 */
static void register_object(UAVObjHandle obj)
{
	uint16_t period;

	if (settings.Profile == LOGGINGSETTINGS_PROFILE_FULLBORE) {
		if (UAVObjIsSettings(obj)) {
			return;
		}

		period = 1;
	} else {
		UAVObjMetadata meta_data;
		if (UAVObjGetMetadata(obj, &meta_data) < 0){
			return;
		}

		if (meta_data.loggingUpdatePeriod == 0){
			return;
		}

		period = meta_data.loggingUpdatePeriod;
	}

	period = MAX(period, get_minimum_logging_period());

	if (period == 1) {
		// log every update
		UAVObjConnectCallback(obj, obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED);
	} else {
		// log updates throttled
		UAVObjConnectCallbackThrottled(obj, obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, period);
	}
}

/**
 * Register objects for the default logging profile
 */
static void register_default_profile()
{
	// For the default profile, we limit things to 100Hz (for now)
	uint16_t min_period = MAX(get_minimum_logging_period(), 10);

	// Objects for which we log all changes (use 100Hz to limit max data rate)
	UAVObjConnectCallbackThrottled(FlightStatusHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 10);
	UAVObjConnectCallbackThrottled(SystemAlarmsHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 10);
	if (WaypointActiveHandle()) {
		UAVObjConnectCallbackThrottled(WaypointActiveHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 10);
	}

	if (SystemIdentHandle()){
		UAVObjConnectCallbackThrottled(SystemIdentHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 10);
	}

	// Log fast
	UAVObjConnectCallbackThrottled(AccelsHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, min_period);
	UAVObjConnectCallbackThrottled(GyrosHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, min_period);

	// Log a bit slower
	UAVObjConnectCallbackThrottled(AttitudeActualHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 5 * min_period);

	if (MagnetometerHandle()) {
		UAVObjConnectCallbackThrottled(MagnetometerHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 5 * min_period);
	}

	UAVObjConnectCallbackThrottled(ManualControlCommandHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 5 * min_period);
	UAVObjConnectCallbackThrottled(ActuatorDesiredHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 5 * min_period);
	UAVObjConnectCallbackThrottled(StabilizationDesiredHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 5 * min_period);

	// Log slow
	if (FlightBatteryStateHandle()) {
		UAVObjConnectCallbackThrottled(FlightBatteryStateHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 10 * min_period);
	}
	if (BaroAltitudeHandle()) {
		UAVObjConnectCallbackThrottled(BaroAltitudeHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 10 * min_period);
	}
	if (AirspeedActualHandle()) {
		UAVObjConnectCallbackThrottled(AirspeedActualHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 10 * min_period);
	}
	if (GPSPositionHandle()) {
		UAVObjConnectCallbackThrottled(GPSPositionHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 10 * min_period);
	}
	if (PositionActualHandle()) {
		UAVObjConnectCallbackThrottled(PositionActualHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 10 * min_period);
	}
	if (VelocityActualHandle()) {
		UAVObjConnectCallbackThrottled(VelocityActualHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 10 * min_period);
	}

	// Log very slow
	if (GPSTimeHandle()) {
		UAVObjConnectCallbackThrottled(GPSTimeHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 50 * min_period);
	}

	// Log very very slow
	if (GPSSatellitesHandle()) {
		UAVObjConnectCallbackThrottled(GPSSatellitesHandle(), obj_updated_callback, NULL, EV_UPDATED | EV_UNPACKED, 500 * min_period);
	}
}


/**
 * Write log file header
 * see firmwareinfotemplate.c
 */
static void writeHeader()
{
	uint8_t fw_blob[100];
	PIOS_BL_HELPER_FLASH_Read_Description((uint8_t *) &fw_blob,
			sizeof(fw_blob));

	int pos;
#define STR_BUF_LEN 45
	char tmp_str[STR_BUF_LEN];
	char *info_str;
	char this_char;
	DateTimeT date_time;

	// Header
	#define LOG_HEADER "dRonin git hash:\n"
	send_data((uint8_t *)LOG_HEADER, strlen(LOG_HEADER));

	// Commit tag name
	// XXX all of thse should use the fw_version_info structure instead of
	// doing offset math, but we need to factor out the fw_version_info
	// struct from the templates first.
	info_str = (char*)(fw_blob + 14);
	send_data((uint8_t*)info_str, strnlen(info_str, 26));

	// Git commit hash
	pos = 0;
	tmp_str[pos++] = ':';
	for (int i = 0; i < 4; i++){
		this_char = *(char*)(fw_blob + 7 - i);
		tmp_str[pos++] = DIGITS[(this_char & 0xF0) >> 4];
		tmp_str[pos++] = DIGITS[(this_char & 0x0F)];
	}
	send_data((uint8_t*)tmp_str, pos);

	// Date
	date_from_timestamp(*(uint32_t *)(fw_blob + 8), &date_time);
	uint8_t len = snprintf(tmp_str, STR_BUF_LEN, " %d%02d%02d\n", 1900 + date_time.year, date_time.mon + 1, date_time.mday);
	send_data((uint8_t*)tmp_str, len);

	// UAVO SHA1
	pos = 0;
	for (int i = 0; i < 20; i++){
		this_char = *(char*)(fw_blob + 60 + i);
		tmp_str[pos++] = DIGITS[(this_char & 0xF0) >> 4];
		tmp_str[pos++] = DIGITS[(this_char & 0x0F)];
	}
	tmp_str[pos++] = '\n';
	send_data((uint8_t*)tmp_str, pos);
}

static void updateSettings()
{
	if (logging_com_id) {

		// Retrieve settings
		uint8_t speed;
		ModuleSettingsOpenLogSpeedGet(&speed);

		// Set port speed
		switch (speed) {
		case MODULESETTINGS_OPENLOGSPEED_115200:
			PIOS_COM_ChangeBaud(logging_com_id, 115200);
			break;
		case MODULESETTINGS_OPENLOGSPEED_250000:
			PIOS_COM_ChangeBaud(logging_com_id, 250000);
			break;
		/* Serial @ 42MHz 16OS can precisely hit 2mbps, 1.5mbps;
		 * Serial @ 45MHz 16OS can precisely hit 1.5mbps (2mbps off by 2%)
		 * Serial @ 48MHz 16OS can precisely hit 2mbps, 1.5mbps
		 */
		case MODULESETTINGS_OPENLOGSPEED_1500000:
			PIOS_COM_ChangeBaud(logging_com_id, 1500000);
			break;
		case MODULESETTINGS_OPENLOGSPEED_2000000:
			PIOS_COM_ChangeBaud(logging_com_id, 2000000);
			break;
		/* This is the weird one.
		 * Serial @ 42MHz will pick 2470588 - 0.4% from openlager
		 * Serial @ 45MHz will pick 2500000 - 1.6% away
		 * Serial @ 96MHz (OpenLager) will pick 2461538
		 * Tolerance is supposed to be 3.3%-ish, but this is a
		 * high-noise environment and a fast signal, so..
		 */
		case MODULESETTINGS_OPENLOGSPEED_2470000:
			PIOS_COM_ChangeBaud(logging_com_id, 2470000);
			break;
		}
	}
}

/**
  * @}
  * @}
  */
