/**
 ******************************************************************************
 * @file       buzzer.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup dRoninModules dRonin Modules
 * @{
 * @addtogroup Buzzer Buzzer Module
 * @{
 * @brief      Generates visible and audible alarms
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

#include "openpilot.h"

#include "flightbuzzersettings.h"
#include "modulesettings.h"
#include "flightstatus.h"
#include "accessorydesired.h"
#include "pios_thread.h"

// ****************
// Private constants
#define STACK_SIZE_BYTES            430
#define TASK_PRIORITY               PIOS_THREAD_PRIO_LOW
#define SAMPLE_PERIOD_MS            200
// Private types

// Private variables
static bool module_enabled = false;
static struct pios_thread *buzzerTaskHandle;
static bool buzzer_settings_updated;

// ****************
// Private functions
static void buzzerTask(void * parameters);
static uint32_t getTune(uint8_t arming, bool was_armed, bool was_disarmed, FlightBuzzerSettingsData buzzerSettings);

static int32_t BuzzerStart(void)
{
	if (module_enabled) {
		FlightBuzzerSettingsConnectCallbackCtx(UAVObjCbSetFlag, &buzzer_settings_updated);

		// Start tasks
		buzzerTaskHandle = PIOS_Thread_Create(buzzerTask, "buzzerBridge", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
		TaskMonitorAdd(TASKINFO_RUNNING_BUZZER, buzzerTaskHandle);
		return 0;
	}
	return -1;
}
/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t BuzzerInitialize(void)
{
#ifdef MODULE_Buzzer_BUILTIN
	module_enabled = true;
#else
	uint8_t module_state[MODULESETTINGS_ADMINSTATE_NUMELEM];
	ModuleSettingsAdminStateGet(module_state);
	if (module_state[MODULESETTINGS_ADMINSTATE_BUZZER] == MODULESETTINGS_ADMINSTATE_ENABLED) {
		module_enabled = true;
	} else {
		module_enabled = false;
		return 0;
	}
#endif
	FlightBuzzerSettingsInitialize();

	return 0;
}
MODULE_INITCALL(BuzzerInitialize, BuzzerStart);

/**
 * Main task. It does not return.
 */
static void buzzerTask(void * parameters)
{
	FlightBuzzerSettingsData buzzerSettings;
	
	buzzer_settings_updated = true;
	bool was_armed = false;
	bool was_disarmed = true;

	// Main task loop
	uint32_t lastSysTime;
	lastSysTime = PIOS_Thread_Systime();
	
	while (true) {
		PIOS_Thread_Sleep_Until(&lastSysTime, SAMPLE_PERIOD_MS);

		if (buzzer_settings_updated) {
			buzzer_settings_updated = false;
			FlightBuzzerSettingsGet(&buzzerSettings);
		}


		uint8_t arming;
		FlightStatusArmedGet(&arming);
		
		if ((arming == FLIGHTSTATUS_ARMED_ARMED) && was_disarmed)
			was_armed = true;

		if ((arming == FLIGHTSTATUS_ARMED_DISARMED) && was_armed)
			was_disarmed = true;


		static uint32_t currTune = 0;
		static uint32_t currTuneState = 0;

		//get a new tune
		uint32_t newTune = getTune(arming, was_armed, was_disarmed, buzzerSettings);
		
		//make sure arming is only triggered once
		if ((arming == FLIGHTSTATUS_ARMED_ARMED) && was_disarmed)
			was_disarmed = false;

		// Do we need to change tune?
		if ((newTune != 0) && (currTuneState == 0)) {
			currTune = newTune;
			currTuneState = currTune;
		}


		// Play tune
		bool buzzOn = false;

		if (currTune) {
			buzzOn = (currTuneState&1);  // Buzz when the LS bit is 1

			// Go to next bit in alarm_seq_state
			currTuneState >>= 1;
		}
		
		if (buzzOn)
			PIOS_GPIO_Off(buzzerSettings.BuzzerPin);  //needs a inversion check for npn/pnp transistor
		else
			PIOS_GPIO_On(buzzerSettings.BuzzerPin);

	}
}


static uint32_t getTune(uint8_t arming, bool was_armed, bool was_disarmed, FlightBuzzerSettingsData buzzerSettings)
{
	//Failsafe
	if (((buzzerSettings.Alarms[FLIGHTBUZZERSETTINGS_ALARMS_FAILSAFE] == FLIGHTBUZZERSETTINGS_ALARMS_WASARMED) && was_armed)
			|| (buzzerSettings.Alarms[FLIGHTBUZZERSETTINGS_ALARMS_FAILSAFE] == FLIGHTBUZZERSETTINGS_ALARMS_ON)) {
		
		uint8_t mode;
		FlightStatusFlightModeGet(&mode);
		
		if(mode == FLIGHTSTATUS_FLIGHTMODE_FAILSAFE) {
			uint32_t newTune=0b1100000;
			return newTune;
			}
	}
	//Accessory
	if (((buzzerSettings.Alarms[FLIGHTBUZZERSETTINGS_ALARMS_ACCESSORY] == FLIGHTBUZZERSETTINGS_ALARMS_WASARMED) && was_armed)
			|| (buzzerSettings.Alarms[FLIGHTBUZZERSETTINGS_ALARMS_ACCESSORY] == FLIGHTBUZZERSETTINGS_ALARMS_ON)) {
		
		AccessoryDesiredData acc;
		AccessoryDesiredInstGet(buzzerSettings.AcessoryChan, &acc);
		
		if(acc.AccessoryVal > 0.5f) {
			uint32_t newTune=0b1100000;
			return newTune;
		}
	}
	//Arming
	if (((buzzerSettings.Alarms[FLIGHTBUZZERSETTINGS_ALARMS_ARMING] == FLIGHTBUZZERSETTINGS_ALARMS_WASARMED) && was_armed)
			|| (buzzerSettings.Alarms[FLIGHTBUZZERSETTINGS_ALARMS_ARMING] == FLIGHTBUZZERSETTINGS_ALARMS_ON)) {
		
		if((arming == FLIGHTSTATUS_ARMED_ARMED) && was_disarmed) {
			uint32_t newTune=0b1010;
			return newTune;
		}
	}
	//Flightmode
	if (((buzzerSettings.Alarms[FLIGHTBUZZERSETTINGS_ALARMS_FLIGHTMODE] == FLIGHTBUZZERSETTINGS_ALARMS_WASARMED) && was_armed)
			|| (buzzerSettings.Alarms[FLIGHTBUZZERSETTINGS_ALARMS_FLIGHTMODE] == FLIGHTBUZZERSETTINGS_ALARMS_ON)) {
		
		uint8_t mode;
		static uint8_t lastMode;
		FlightStatusFlightModeGet(&mode);
		
		if((mode != lastMode) && (mode != FLIGHTSTATUS_FLIGHTMODE_FAILSAFE)) {
			uint32_t newTune=0b10;
			lastMode = mode;
			return newTune;
		}
	}
	//Battery
	if (((buzzerSettings.Alarms[FLIGHTBUZZERSETTINGS_ALARMS_BATWAR] == FLIGHTBUZZERSETTINGS_ALARMS_WASARMED) && was_armed)
			|| (buzzerSettings.Alarms[FLIGHTBUZZERSETTINGS_ALARMS_BATWAR] == FLIGHTBUZZERSETTINGS_ALARMS_ON)) {
		
		if(AlarmsGet(SYSTEMALARMS_ALARM_BATTERY) == SYSTEMALARMS_ALARM_WARNING) {
			uint32_t newTune=0b100100100;
			return newTune;
		}
		else if (AlarmsGet(SYSTEMALARMS_ALARM_BATTERY) > SYSTEMALARMS_ALARM_WARNING) {
			uint32_t newTune=0b100100100;
			return newTune;
		}
	}
	//Gps
	if (((buzzerSettings.Alarms[FLIGHTBUZZERSETTINGS_ALARMS_GPSWAR] == FLIGHTBUZZERSETTINGS_ALARMS_WASARMED) && was_armed)
			|| (buzzerSettings.Alarms[FLIGHTBUZZERSETTINGS_ALARMS_GPSWAR] == FLIGHTBUZZERSETTINGS_ALARMS_ON)) {
		
		if (AlarmsGet(SYSTEMALARMS_ALARM_GPS) == SYSTEMALARMS_ALARM_WARNING) {
			uint32_t newTune=0b100100100;
			return newTune;
		}
		else if (AlarmsGet(SYSTEMALARMS_ALARM_GPS) > SYSTEMALARMS_ALARM_WARNING) {
			uint32_t newTune=0b100100100;
			return newTune ;
		}
	}
	
	//no tune found
	return 0;
}

/**
  * @}
  * @}
  */
