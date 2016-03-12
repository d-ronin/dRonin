/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{ 
 * @addtogroup GPSModule GPS Module
 * @{ 
 *
 * @file       GPS.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 *
 * @brief      GPS module, handles UBX and NMEA streams from GPS
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

// ****************

#include "openpilot.h"
#include "GPS.h"

#include "gpsposition.h"
#include "airspeedactual.h"
#include "homelocation.h"
#include "gpstime.h"
#include "gpssatellites.h"
#include "gpsvelocity.h"
#include "modulesettings.h"
#include "pios_thread.h"
#include "pios_modules.h"

#include "NMEA.h"
#include "UBX.h"
#include "ubx_cfg.h"

#include <pios_hal.h>

#if defined(PIOS_GPS_PROVIDES_AIRSPEED)
#include "gps_airspeed.h"
#endif

// ****************
// Private functions

static void gpsTask(void *parameters);
static void updateSettings();

// ****************
// Private constants

#define GPS_TIMEOUT_MS                  750
#define GPS_COM_TIMEOUT_MS              100


#if defined(PIOS_GPS_MINIMAL)
	#define STACK_SIZE_BYTES            500
#else
	#define STACK_SIZE_BYTES            850
#endif // PIOS_GPS_MINIMAL

#define TASK_PRIORITY                   PIOS_THREAD_PRIO_LOW

// ****************
// Private variables

static uint32_t gpsPort;
static bool module_enabled = false;

static struct pios_thread *gpsTaskHandle;

static char* gps_rx_buffer;

static struct GPS_RX_STATS gpsRxStats;

// ****************
/**
 * Initialise the gps module
 * \return -1 if initialisation failed
 * \return 0 on success
 */

int32_t GPSStart(void)
{
	if (module_enabled) {
		if (gpsPort) {
			// Start gps task
			gpsTaskHandle = PIOS_Thread_Create(gpsTask, "GPS", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
			TaskMonitorAdd(TASKINFO_RUNNING_GPS, gpsTaskHandle);
			return 0;
		}

		AlarmsSet(SYSTEMALARMS_ALARM_GPS, SYSTEMALARMS_ALARM_ERROR);
	}
	return -1;
}

/**
 * Initialise the gps module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t GPSInitialize(void)
{
	gpsPort = PIOS_COM_GPS;
	uint8_t	gpsProtocol;

#ifdef MODULE_GPS_BUILTIN
	module_enabled = true;
#else
	module_enabled = PIOS_Modules_IsEnabled(PIOS_MODULE_GPS);
#endif

	// These things are only conditional on small F1 targets.
	// Expected to be always present otherwise.
#ifdef SMALLF1
	if (gpsPort && module_enabled) {
#endif
		GPSPositionInitialize();
		GPSVelocityInitialize();
#if !defined(PIOS_GPS_MINIMAL)
		GPSTimeInitialize();
		GPSSatellitesInitialize();
		HomeLocationInitialize();
		UBloxInfoInitialize();
#endif
#if defined(PIOS_GPS_PROVIDES_AIRSPEED)
		AirspeedActualInitialize();
#endif
		updateSettings();
#ifdef SMALLF1
	}
#endif

	if (gpsPort && module_enabled) {
		ModuleSettingsGPSDataProtocolGet(&gpsProtocol);
		switch (gpsProtocol) {
			case MODULESETTINGS_GPSDATAPROTOCOL_NMEA:
				gps_rx_buffer = PIOS_malloc(NMEA_MAX_PACKET_LENGTH);
				break;
			case MODULESETTINGS_GPSDATAPROTOCOL_UBX:
				gps_rx_buffer = PIOS_malloc(sizeof(struct UBXPacket));
				break;
			default:
				gps_rx_buffer = NULL;
		}
		PIOS_Assert(gps_rx_buffer);

		return 0;
	}

	return -1;
}

MODULE_INITCALL(GPSInitialize, GPSStart);

// ****************

static void gpsConfigure(uint8_t gpsProtocol)
{
	ModuleSettingsGPSAutoConfigureOptions gpsAutoConfigure;
	ModuleSettingsGPSAutoConfigureGet(&gpsAutoConfigure);

	if (gpsAutoConfigure != MODULESETTINGS_GPSAUTOCONFIGURE_TRUE) {
		return;
	}

#if !defined(PIOS_GPS_MINIMAL)
	switch (gpsProtocol) {
#if defined(PIOS_INCLUDE_GPS_UBX_PARSER)
		case MODULESETTINGS_GPSDATAPROTOCOL_UBX:
		{
			// Runs through a number of possible GPS baud rates to
			// configure the ublox baud rate. This uses a NMEA string
			// so could work for either UBX or NMEA actually. This is
			// somewhat redundant with updateSettings below, but that
			// is only called on startup and is not an issue.

			ModuleSettingsGPSSpeedOptions baud_rate;
			ModuleSettingsGPSConstellationOptions constellation;
			ModuleSettingsGPSSBASConstellationOptions sbas_const;
			ModuleSettingsGPSDynamicsModeOptions dyn_mode;

			ModuleSettingsGPSSpeedGet(&baud_rate);
			ModuleSettingsGPSConstellationGet(&constellation);
			ModuleSettingsGPSSBASConstellationGet(&sbas_const);
			ModuleSettingsGPSDynamicsModeGet(&dyn_mode);

			ubx_cfg_set_baudrate(gpsPort, baud_rate);

			PIOS_Thread_Sleep(1000);

			ubx_cfg_send_configuration(gpsPort, gps_rx_buffer,
					constellation, sbas_const, dyn_mode);
		}
		break;
#endif
	}
#endif /* PIOS_GPS_MINIMAL */
}

/**
 * Main gps task. It does not return.
 */

static void gpsTask(void *parameters)
{
	GPSPositionData gpsposition;

	uint32_t timeOfLastUpdateMs = 0;
	uint32_t timeOfConfigAttemptMs = 0;

	uint8_t	gpsProtocol;

#ifdef PIOS_GPS_PROVIDES_AIRSPEED
	gps_airspeed_initialize();
#endif

	GPSPositionGet(&gpsposition);

	// Wait for power to stabilize before talking to external devices
	PIOS_Thread_Sleep(1000);

	// Loop forever
	while (1) {
		uint32_t xDelay = GPS_COM_TIMEOUT_MS;

		uint32_t loopTimeMs = PIOS_Thread_Systime();

		// XXX TODO: also on modulesettings change..
		if (!timeOfConfigAttemptMs) {
			ModuleSettingsGPSDataProtocolGet(&gpsProtocol);

			gpsConfigure(gpsProtocol);
			timeOfConfigAttemptMs = PIOS_Thread_Systime();

			continue;
		}

		uint8_t c;

		// This blocks the task until there is something on the buffer
		while (PIOS_COM_ReceiveBuffer(gpsPort, &c, 1, xDelay) > 0)
		{
			int res;
			switch (gpsProtocol) {
#if defined(PIOS_INCLUDE_GPS_NMEA_PARSER)
				case MODULESETTINGS_GPSDATAPROTOCOL_NMEA:
					res = parse_nmea_stream (c,gps_rx_buffer, &gpsposition, &gpsRxStats);
					break;
#endif
#if defined(PIOS_INCLUDE_GPS_UBX_PARSER)
				case MODULESETTINGS_GPSDATAPROTOCOL_UBX:
					res = parse_ubx_stream (c,gps_rx_buffer, &gpsposition, &gpsRxStats);
					break;
#endif
				default:
					res = NO_PARSER; // this should not happen
					break;
			}

			if (res == PARSER_COMPLETE) {
				timeOfLastUpdateMs = loopTimeMs;
			}

			xDelay = 0;	// For now on, don't block / wait,
					// but consume what we can from the fifo
		}

		// Check for GPS timeout
		if ((loopTimeMs - timeOfLastUpdateMs) >= GPS_TIMEOUT_MS) {
			// we have not received any valid GPS sentences for a while.
			// either the GPS is not plugged in or a hardware problem or the GPS has locked up.
			uint8_t status = GPSPOSITION_STATUS_NOGPS;
			GPSPositionStatusSet(&status);
			AlarmsSet(SYSTEMALARMS_ALARM_GPS, SYSTEMALARMS_ALARM_ERROR);
			/* Don't reinitialize too often. */
			if ((loopTimeMs - timeOfConfigAttemptMs) >= GPS_TIMEOUT_MS) {
				timeOfConfigAttemptMs = 0; // reinit next loop
			}
		} else {
			// we appear to be receiving GPS sentences OK, we've had an update
			//criteria for GPS-OK taken from this post
			if (gpsposition.PDOP < 3.5f && 
			    gpsposition.Satellites >= 7 &&
			    (gpsposition.Status == GPSPOSITION_STATUS_FIX3D ||
			         gpsposition.Status == GPSPOSITION_STATUS_DIFF3D)) {
				AlarmsClear(SYSTEMALARMS_ALARM_GPS);
			} else if (gpsposition.Status == GPSPOSITION_STATUS_FIX3D ||
			           gpsposition.Status == GPSPOSITION_STATUS_DIFF3D)
						AlarmsSet(SYSTEMALARMS_ALARM_GPS, SYSTEMALARMS_ALARM_WARNING);
					else
						AlarmsSet(SYSTEMALARMS_ALARM_GPS, SYSTEMALARMS_ALARM_CRITICAL);
		}

	}
}


/**
 * Update the GPS settings, called on startup.
 */
static void updateSettings()
{
	if (gpsPort) {

		// Retrieve settings
		uint8_t speed;
		ModuleSettingsGPSSpeedGet(&speed);

		// Set port speed
		PIOS_HAL_ConfigureSerialSpeed(gpsPort, speed);
	}
}

/** 
  * @}
  * @}
  */
