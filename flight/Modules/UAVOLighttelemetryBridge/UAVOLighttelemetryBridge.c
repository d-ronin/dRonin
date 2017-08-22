/**
 ******************************************************************************
 * @addtogroup Modules Modules
 * @{ 
 * @addtogroup UAVOLighttelemetryBridge UAVO to Lighttelemetry Bridge Module
 * @{ 
 *
 * @file	   UAVOLighttelemetryBridge.c
 * @author         dRonin, http://dRonin.org/, Copyright (C) 2016-2017
 * @author	   Tau Labs, http://taulabs.org, Copyright (C) 2013-2016
 *
 * @brief	   Bridges selected UAVObjects to a minimal one way telemetry 
 *			   protocol for really low bitrates (1200/2400 bauds). This can be 
 *			   used with FSK audio modems or increase range for serial telemetry.
 *			   Effective for ground OSD, groundstation HUD and Antenna tracker.
 *			   
 *				Protocol details: 3 different frames, little endian.
 *				  * G Frame (GPS position): 18BYTES
 *					0x24 0x54 0x47 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF	 0xFF	0xC0   
 *					 $	   T	G  --------LAT-------- -------LON---------	SPD --------ALT-------- SAT/FIX	 CRC
 *				  * A Frame (Attitude): 10BYTES
 *					0x24 0x54 0x41 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xC0	
 *					 $	   T   A   --PITCH-- --ROLL--- -HEADING-  CRC
 *				  * S Frame (Sensors): 11BYTES
 *					0x24 0x54 0x53 0xFF 0xFF  0xFF 0xFF	   0xFF	   0xFF		 0xFF		0xC0	 
 *					 $	   T   S   VBAT(mv)	 Current(ma)   RSSI	 AIRSPEED  ARM/FS/FMOD	 CRC
 *
 * Before these used to be explicitly scheduled to time quanta.  Now we just dump
 * out stuff as fast as we can, ensuring the appropriate link share for each
 * frame type.
 *
 * @see		   The GNU Public License (GPL) Version 3
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

#include "accels.h"
#include "airspeedactual.h"
#include "attitudeactual.h"
#include "baroaltitude.h"
#include "flightstatus.h"
#include "gpsposition.h"
#include "flightbatterysettings.h"
#include "flightbatterystate.h"
#include "manualcontrolcommand.h"
#include "positionactual.h"

#include "pios_thread.h"
#include "pios_modules.h"

#include <pios_hal.h>

#if defined(PIOS_INCLUDE_LIGHTTELEMETRY)
// Private constants
#define STACK_SIZE_BYTES 600
#define TASK_PRIORITY PIOS_THREAD_PRIO_LOW

#define CHUNK_TIME 30		/* 30ms. 3.6 bytes @ 1200, 14.4 bytes at 4800 */
/* The expected behavior at 1200 bps becomes then, prepare+send frame, delay 30ms,
 * then prepare 1-3 frames at 30ms intervals that go unsent because of buffer.
 * Then the next one goes.
 */

#define LTM_GFRAME_SIZE 18
#define LTM_AFRAME_SIZE 10
#define LTM_SFRAME_SIZE 11

// Private types

// Private variables
static bool module_enabled = false;
static struct pios_thread *taskHandle;
static uint32_t lighttelemetryPort;
static uint8_t ltm_scheduler;
static uint8_t ltm_slowrate;

// Private functions
static void uavoLighttelemetryBridgeTask(void *parameters);
static void updateSettings();

static int send_LTM_Packet(uint8_t *LTPacket, uint8_t LTPacket_size);
static int send_LTM_Gframe();
static int send_LTM_Aframe();
static int send_LTM_Sframe();


/**
 * Initialise the module, called first on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t uavoLighttelemetryBridgeInitialize()
{
	lighttelemetryPort = PIOS_COM_LIGHTTELEMETRY;

	if (lighttelemetryPort && PIOS_Modules_IsEnabled(PIOS_MODULE_UAVOLIGHTTELEMETRYBRIDGE)) {
		// Update telemetry settings
		module_enabled = true;
		return 0;
	}

	return -1;
}

/**
 * Initialise the module, called on startup after Initialize
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t uavoLighttelemetryBridgeStart()
{
	if ( module_enabled )
	{
		taskHandle = PIOS_Thread_Create(uavoLighttelemetryBridgeTask, "uavoLighttelemetryBridge", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
		TaskMonitorAdd(TASKINFO_RUNNING_UAVOLIGHTTELEMETRYBRIDGE, taskHandle);
		return 0;
	}
	
	return -1;
}

MODULE_INITCALL(uavoLighttelemetryBridgeInitialize, uavoLighttelemetryBridgeStart);


/*#######################################################################
 * Module thread, should not return.
 *#######################################################################
*/

static void uavoLighttelemetryBridgeTask(void *parameters)
{
	updateSettings();
	
	// Main task loop
	while (1)
	{
		int ret = 0;

		if (!ret) {
			switch (ltm_scheduler) {
				case 0:
				case 6:
					ret = send_LTM_Sframe();
					break;

				case 3:
				case 9:
					ret = send_LTM_Gframe();
					break;

				case 1:
				case 4:
				case 7:
				case 10:
					if (ltm_slowrate) {
						break;
					}

				case 2:
				case 5:
				case 8:
				case 11:
					ret = send_LTM_Aframe();
					break;

				default:
					break;
			}
		}

		PIOS_Thread_Sleep(CHUNK_TIME);

		if (ret) {
			/* If we couldn't tx, go around and try the same thing
			 * again.
			 */
			continue;
		}

		ltm_scheduler++;

		if (ltm_scheduler > 11) {
			ltm_scheduler = 0;
		}
	}
}

/*#######################################################################
 * Internal functions
 *#######################################################################
*/
//GPS packet
static int send_LTM_Gframe()
{
	GPSPositionData pdata = { };	/* Set to all 0's */

	bool have_gps = false;

	if (GPSPositionHandle() != NULL) {
		have_gps = true;
		GPSPositionGet(&pdata);
	}

	int32_t lt_latitude = pdata.Latitude;
	int32_t lt_longitude = pdata.Longitude;
	uint8_t lt_groundspeed = (uint8_t)roundf(pdata.Groundspeed); //rounded m/s .
	int32_t lt_altitude = 0;
	if (PositionActualHandle() != NULL) {
		float altitude;
		PositionActualDownGet(&altitude);
		lt_altitude = (int32_t)roundf(altitude * -100.0f);
	} else if (BaroAltitudeHandle() != NULL) {
		float altitude;
		BaroAltitudeAltitudeGet(&altitude);
		lt_altitude = (int32_t)roundf(altitude * 100.0f); //Baro alt in cm.
	} else if (have_gps) {
		lt_altitude = (int32_t)roundf(pdata.Altitude * 100.0f); //GPS alt in cm.
	} else {
		return 0;	/* Don't even bother, no data for this frame! */
	}
	
	uint8_t lt_gpsfix;
	switch (pdata.Status) {
	case GPSPOSITION_STATUS_NOGPS:
		lt_gpsfix = 0;
		break;
	case GPSPOSITION_STATUS_NOFIX:
		lt_gpsfix = 1;
		break;
	case GPSPOSITION_STATUS_FIX2D:
		lt_gpsfix = 2;
		break;
	case GPSPOSITION_STATUS_FIX3D:
		lt_gpsfix = 3;
		break;
	default:
		lt_gpsfix = 0;
		break;
	}
	
	uint8_t lt_gpssats = (int8_t)pdata.Satellites;
	//pack G frame	
	uint8_t LTBuff[LTM_GFRAME_SIZE];
	//G Frame: $T(2 bytes)G(1byte)LAT(cm,4 bytes)LON(cm,4bytes)SPEED(m/s,1bytes)ALT(cm,4bytes)SATS(6bits)FIX(2bits)CRC(xor,1byte)
	//START
	LTBuff[0]  = 0x24; //$
	LTBuff[1]  = 0x54; //T
	//FRAMEID
	LTBuff[2]  = 0x47; //G
	//PAYLOAD
	LTBuff[3]  = (lt_latitude >> 8*0) & 0xFF;
	LTBuff[4]  = (lt_latitude >> 8*1) & 0xFF;
	LTBuff[5]  = (lt_latitude >> 8*2) & 0xFF;
	LTBuff[6]  = (lt_latitude >> 8*3) & 0xFF;
	LTBuff[7]  = (lt_longitude >> 8*0) & 0xFF;
	LTBuff[8]  = (lt_longitude >> 8*1) & 0xFF;
	LTBuff[9]  = (lt_longitude >> 8*2) & 0xFF;
	LTBuff[10] = (lt_longitude >> 8*3) & 0xFF;	
	LTBuff[11] = (lt_groundspeed >> 8*0) & 0xFF;
	LTBuff[12] = (lt_altitude >> 8*0) & 0xFF;
	LTBuff[13] = (lt_altitude >> 8*1) & 0xFF;
	LTBuff[14] = (lt_altitude >> 8*2) & 0xFF;
	LTBuff[15] = (lt_altitude >> 8*3) & 0xFF;
	LTBuff[16] = ((lt_gpssats << 2)& 0xFF ) | (lt_gpsfix & 0b00000011) ; // last 6 bits: sats number, first 2:fix type (0,1,2,3)

	return send_LTM_Packet(LTBuff,LTM_GFRAME_SIZE);
}

//Attitude packet
static int send_LTM_Aframe()
{
	//prepare data
	AttitudeActualData adata;
	AttitudeActualGet(&adata);
	int16_t lt_pitch   = (int16_t)(roundf(adata.Pitch));	//-180/180°
	int16_t lt_roll	   = (int16_t)(roundf(adata.Roll));		//-180/180°
	int16_t lt_heading = (int16_t)(roundf(adata.Yaw));		//-180/180°
	//pack A frame	
	uint8_t LTBuff[LTM_AFRAME_SIZE];
	
	//A Frame: $T(2 bytes)A(1byte)PITCH(2 bytes)ROLL(2bytes)HEADING(2bytes)CRC(xor,1byte)
	//START
	LTBuff[0] = 0x24; //$
	LTBuff[1] = 0x54; //T
	//FRAMEID
	LTBuff[2] = 0x41; //A
	//PAYLOAD
	LTBuff[3] = (lt_pitch >> 8*0) & 0xFF;
	LTBuff[4] = (lt_pitch >> 8*1) & 0xFF;
	LTBuff[5] = (lt_roll >> 8*0) & 0xFF;
	LTBuff[6] = (lt_roll >> 8*1) & 0xFF;
	LTBuff[7] = (lt_heading >> 8*0) & 0xFF;
	LTBuff[8] = (lt_heading >> 8*1) & 0xFF;

	return send_LTM_Packet(LTBuff,LTM_AFRAME_SIZE);
}

//Sensors packet
static int send_LTM_Sframe()
{
	//prepare data
	uint16_t lt_vbat = 0;
	uint16_t lt_amp = 0;
	uint8_t	 lt_rssi = 0;
	uint8_t	 lt_airspeed = 0;
	uint8_t	 lt_arm = 0;
	uint8_t	 lt_failsafe = 0;
	uint8_t	 lt_flightmode = 0;
	
	
	if (FlightBatteryStateHandle() != NULL) {
		FlightBatteryStateData sdata;
		FlightBatteryStateGet(&sdata);
		lt_vbat = (uint16_t)roundf(sdata.Voltage*1000);	  //Battery voltage in mv
		lt_amp = (uint16_t)roundf(sdata.ConsumedEnergy);	  //mA consumed
	}
	if (ManualControlCommandHandle() != NULL) {
		ManualControlCommandData mdata;
		ManualControlCommandGet(&mdata);
		lt_rssi = (uint8_t)mdata.Rssi;					  //RSSI in %
	}
	if (AirspeedActualHandle() != NULL) {
		AirspeedActualData adata;
		AirspeedActualGet (&adata);
		lt_airspeed = (uint8_t)roundf(adata.TrueAirspeed);	  //Airspeed in m/s
	} else if (GPSPositionHandle() != NULL) {
		float groundspeed;
		GPSPositionGroundspeedGet(&groundspeed);
		lt_airspeed = (uint8_t)roundf(groundspeed);
	}

	FlightStatusData fdata;
	FlightStatusGet(&fdata);
	lt_arm = fdata.Armed;									  //Armed status
	if (lt_arm == 1)		//arming , we don't use this one
		lt_arm = 0;		
	else if (lt_arm == 2)  // armed
		lt_arm = 1;
	if (fdata.ControlSource == FLIGHTSTATUS_CONTROLSOURCE_FAILSAFE)
		lt_failsafe = 1;
	else
		lt_failsafe = 0;
	
	// Flight mode(0-19): 0: Manual, 1: Rate, 2: Attitude/Angle, 3: Horizon, 4: Acro, 5: Stabilized1, 6: Stabilized2, 7: Stabilized3,
	// 8: Altitude Hold, 9: Loiter/GPS Hold, 10: Auto/Waypoints, 11: Heading Hold / headFree,
	// 12: Circle, 13: RTH, 14: FollowMe, 15: LAND, 16:FlybyWireA, 17: FlybywireB, 18: Cruise, 19: Unknown

	switch (fdata.FlightMode) {
	case FLIGHTSTATUS_FLIGHTMODE_MANUAL:
		lt_flightmode = 0; break;
	case FLIGHTSTATUS_FLIGHTMODE_STABILIZED1:
		lt_flightmode = 5; break;
	case FLIGHTSTATUS_FLIGHTMODE_STABILIZED2:
		lt_flightmode = 6; break;
	case FLIGHTSTATUS_FLIGHTMODE_STABILIZED3:
		lt_flightmode = 7; break;
	case FLIGHTSTATUS_FLIGHTMODE_ALTITUDEHOLD:
		lt_flightmode = 8; break;
	case FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD:
		lt_flightmode = 9; break;
	case FLIGHTSTATUS_FLIGHTMODE_RETURNTOHOME:
		lt_flightmode = 13; break;
	case FLIGHTSTATUS_FLIGHTMODE_PATHPLANNER:
		lt_flightmode = 10; break;
	default:
		lt_flightmode = 19; //Unknown
	}
	//pack A frame	
	uint8_t LTBuff[LTM_SFRAME_SIZE];
	
	//A Frame: $T(2 bytes)A(1byte)PITCH(2 bytes)ROLL(2bytes)HEADING(2bytes)CRC(xor,1byte)
	//START
	LTBuff[0] = 0x24; //$
	LTBuff[1] = 0x54; //T
	//FRAMEID
	LTBuff[2] = 0x53; //S
	//PAYLOAD
	LTBuff[3] = (lt_vbat >> 8*0) & 0xFF;
	LTBuff[4] = (lt_vbat >> 8*1) & 0xFF;
	LTBuff[5] = (lt_amp >> 8*0) & 0xFF;
	LTBuff[6] = (lt_amp >> 8*1) & 0xFF;
	LTBuff[7] = (lt_rssi >> 8*0) & 0xFF;
	LTBuff[8] = (lt_airspeed >> 8*0) & 0xFF;
	LTBuff[9] = ((lt_flightmode << 2)& 0xFF ) | ((lt_failsafe << 1)& 0b00000010 ) | (lt_arm & 0b00000001) ; // last 6 bits: flight mode, 2nd bit: failsafe, 1st bit: Arm status.

	return send_LTM_Packet(LTBuff,LTM_SFRAME_SIZE);
}

static int send_LTM_Packet(uint8_t *LTPacket, uint8_t LTPacket_size)
{
	//calculate Checksum
	uint8_t LTCrc = 0x00;
	for (int i = 3; i < LTPacket_size-1; i++) {
		LTCrc ^= LTPacket[i];
	}
	LTPacket[LTPacket_size-1] = LTCrc;

	int ret = PIOS_COM_SendBufferNonBlocking(lighttelemetryPort,
			LTPacket, LTPacket_size);

	if (ret < 0) {
		return -1;
	}

	return 0;
}

static void updateSettings()
{
	// Retrieve settings
	uint8_t speed;
	ModuleSettingsLightTelemetrySpeedGet(&speed);

	PIOS_HAL_ConfigureSerialSpeed(lighttelemetryPort, speed);

	if (speed == MODULESETTINGS_LIGHTTELEMETRYSPEED_1200)
		ltm_slowrate = 1;
	else 
		ltm_slowrate = 0;
}
#endif //end define lighttelemetry
/**
 * @}
 * @}
 */
