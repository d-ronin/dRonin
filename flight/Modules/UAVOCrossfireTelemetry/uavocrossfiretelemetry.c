/**
 ******************************************************************************
 * @addtogroup TauLabsModules TauLabs Modules
 * @{ 
 * @addtogroup UAVOCrossfireTelemetry UAVO to TBS Crossfire telemetry converter
 * @{ 
 *
 * @file       uavocrossfiretelemetry.c
 * @author     dRonin, http://dronin.org, Copyright (C) 2017
 * @brief      Bridges selected UAVObjects to the TBS Crossfire
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

#include "pios.h"
#if defined(PIOS_INCLUDE_CROSSFIRE)

#include "openpilot.h"
#include "pios_hal.h"
#include "pios_thread.h"
#include "pios_modules.h"
#include "pios_crc.h"
#include "taskmonitor.h"
#include "taskinfo.h"

#include "uavocrossfiretelemetry.h"

#include "modulesettings.h"
#include "flightbatterystate.h"
#include "flightbatterysettings.h"
#include "gpsposition.h"
#include "attitudeactual.h"
#include "manualcontrolsettings.h"

// Private constants
#define STACK_SIZE_BYTES 600		// Reevaluate.
#define TASK_PRIORITY				PIOS_THREAD_PRIO_LOW

#define DEG2RAD(x) ((float)x * (float)M_PI / 180.0f)

// Private variables
static struct pios_thread *uavoCrossfireTelemetryTaskHandle;
static bool module_enabled;

// Crossfire receiver device
static uintptr_t crsf_telem_dev_id;

static void uavoCrossfireTelemetryTask(void *parameters);

/**
 * start the module
 * \return -1 if start failed
 * \return 0 on success
 */
static int32_t uavoCrossfireTelemetryStart(void)
{
	uintptr_t rcvr = PIOS_HAL_GetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_TBSCROSSFIRE);

	// We shouldn't need to check whether it's a valid Crossfire device, because if
	// something else was read out of the channel group, something's really borked.
	if(rcvr) {
		crsf_telem_dev_id = PIOS_RCVR_GetLowerDevice(rcvr);
		if (module_enabled && (PIOS_Crossfire_InitTelemetry(crsf_telem_dev_id) == 0)) {
			// Start task
			uavoCrossfireTelemetryTaskHandle = PIOS_Thread_Create(
					uavoCrossfireTelemetryTask, "uavoCrossfireTelemetry",
					STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
			TaskMonitorAdd(TASKINFO_RUNNING_UAVOCROSSFIRETELEMETRY,
					uavoCrossfireTelemetryTaskHandle);
			return 0;
		}
	}

	return -1;
}

static int32_t uavoCrossfireTelemetryInitialize(void)
{
	module_enabled = PIOS_Modules_IsEnabled(PIOS_MODULE_UAVOCROSSFIRETELEMETRY); 
	return 0;
}
MODULE_INITCALL(uavoCrossfireTelemetryInitialize, uavoCrossfireTelemetryStart)

// Stupid big endian stuff.
#define WRITE_VAL(buf,p,x)				{ typeof(x) v = x; uint8_t *q = (uint8_t*)&v, n = sizeof(v); do{ buf[p++] = q[--n]; } while(n); }
#define WRITE_VAL16(buf,p,x)			{ typeof(x) v = x; uint8_t *q = (uint8_t*)&v; buf[p++] = q[1]; buf[p++] = q[0]; }
#define WRITE_VAL32(buf,p,x)			{ typeof(x) v = x; uint8_t *q = (uint8_t*)&v; buf[p++] = q[3]; buf[p++] = q[2]; buf[p++] = q[1]; buf[p++] = q[0]; }

static int crsftelem_create_attitude(uint8_t *buf)
{
	int ptr = 0;

	if(AttitudeActualHandle()) {
		AttitudeActualData attitudeData;
		AttitudeActualGet(&attitudeData);

		buf[ptr++] = 0;
		buf[ptr++] = CRSF_PAYLOAD_LEN(CRSF_PAYLOAD_ATTITUDE);
		buf[ptr++] = CRSF_FRAME_ATTITUDE;

		WRITE_VAL16(buf, ptr, (int16_t)(DEG2RAD(attitudeData.Pitch)*10000.0f));
		WRITE_VAL16(buf, ptr, (int16_t)(DEG2RAD(attitudeData.Roll)*10000.0f));
		WRITE_VAL16(buf, ptr, (int16_t)(DEG2RAD(attitudeData.Yaw)*10000.0f));

		buf[ptr++] = PIOS_CRC_updateCRC_TBS(0, buf+2, buf[1] - CRSF_CRC_LEN);
	}

	return ptr;
}

static int crsftelem_create_battery(uint8_t *buf)
{
	int ptr = 0;

	if(FlightBatteryStateHandle() && FlightBatterySettingsHandle()) {
		FlightBatteryStateData batteryData;
		FlightBatterySettingsData batterySettings;

		FlightBatteryStateGet(&batteryData);
		FlightBatterySettingsGet(&batterySettings);

		buf[ptr++] = 0;
		buf[ptr++] = CRSF_PAYLOAD_LEN(CRSF_PAYLOAD_BATTERY);
		buf[ptr++] = CRSF_FRAME_BATTERY;

		WRITE_VAL16(buf, ptr, (uint16_t)(batteryData.Voltage * 10.0f))
		WRITE_VAL16(buf, ptr, (uint16_t)(batteryData.Current * 10.0f))

		// Should apparently be capacity used?
		buf[ptr++] = (uint8_t)((batterySettings.Capacity & 0x00FF0000) >> 16);
		buf[ptr++] = (uint8_t)((batterySettings.Capacity & 0x0000FF00) >> 8);
		buf[ptr++] = (uint8_t)(batterySettings.Capacity & 0x000000FF);

		float charge_state = batterySettings.Capacity == 0 ? 100.0f : (batteryData.ConsumedEnergy / batterySettings.Capacity);
		if(charge_state < 0) charge_state = 0;
		else if(charge_state > 100) charge_state = 100;
		buf[ptr++] = (uint8_t)charge_state;

		buf[ptr++] = PIOS_CRC_updateCRC_TBS(0, buf+2, buf[1] - CRSF_CRC_LEN);
	}

	return ptr;
}

static int crsftelem_create_gps(uint8_t *buf)
{
	int ptr = 0;

	if(GPSPositionHandle()) {
		GPSPositionData gpsData;
		GPSPositionGet(&gpsData);

		if(gpsData.Status >= GPSPOSITION_STATUS_FIX2D) {
			buf[ptr++] = 0;
			buf[ptr++] = CRSF_PAYLOAD_LEN(CRSF_PAYLOAD_GPS);
			buf[ptr++] = CRSF_FRAME_GPS;

			// Latitude (x10^7, as dRonin)
			WRITE_VAL32(buf, ptr, (int32_t)gpsData.Latitude);
			// Longitude (x10^7, as dRonin)
			WRITE_VAL32(buf, ptr, (int32_t)gpsData.Longitude);
			// Groundspeed (apparently tenth of km/h)
			WRITE_VAL16(buf, ptr, (uint16_t)(gpsData.Groundspeed*10.0f));
			// Heading (apparently hundreth of a degree)
			WRITE_VAL16(buf, ptr, (uint16_t)(gpsData.Heading*100.0f));
			// Altitude 1000 = 0m
			WRITE_VAL16(buf, ptr, (uint16_t)(1000.0f+
				(gpsData.Status >= GPSPOSITION_STATUS_FIX3D ? gpsData.Altitude : 0.0f)));
			// Satellites
			buf[ptr++] = gpsData.Satellites;

			buf[ptr++] = PIOS_CRC_updateCRC_TBS(0, buf+2, buf[1] - CRSF_CRC_LEN);
		}
	}

	return ptr;
}

static void uavoCrossfireTelemetryTask(void *parameters)
{
	// Three objects, each at UPDATE_HZ
	uint32_t idledelay = 1000 / (3 * UPDATE_HZ);
	uint8_t counter = 0;

	// Wait for stuff to setup?
	PIOS_Thread_Sleep(1000);

	while (1) {
		uint8_t buf[CRSF_MAX_FRAMELEN];
		uint8_t len = 0;

		if(!PIOS_Crossfire_IsFailsafed(crsf_telem_dev_id)) {

			switch(counter++ % 3) {
				default:
				case 0: // Attitude
					len = crsftelem_create_attitude(buf);
					break;
				case 1: // Battery
					len = crsftelem_create_battery(buf);
					break;
				case 2: // GPS
					len = crsftelem_create_gps(buf);
					break;
			}

			if(len) {
				while(PIOS_Crossfire_SendTelemetry(crsf_telem_dev_id, buf, len)) {
					// Keep repeating until telemetry went through, without
					// locking up the whole thing.
					PIOS_Thread_Sleep(2);
				}
			}
		}

		PIOS_Thread_Sleep(idledelay);
	}
}


#endif // PIOS_INCLUDE_CROSSFIRE
