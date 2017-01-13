/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup SystemModule System Module
 * @{
 *
 * @file       systemmod.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2015
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @brief      System module
 *
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
 */

#include "openpilot.h"
#include <eventdispatcher.h>
#include <utlist.h>

#include "systemmod.h"
#include "sanitycheck.h"
#include "taskinfo.h"
#include "taskmonitor.h"
#include "pios_thread.h"
#include "pios_mutex.h"
#include "pios_queue.h"
#include "misc_math.h"
#include "morsel.h"

#include "annunciatorsettings.h"
#include "flightstatus.h"
#include "manualcontrolsettings.h"
#include "objectpersistence.h"
#include "rfm22bstatus.h"
#include "stabilizationsettings.h"
#include "stateestimation.h"
#include "systemsettings.h"
#include "systemstats.h"
#include "watchdogstatus.h"

#ifdef SYSTEMMOD_RGBLED_SUPPORT
#include "rgbledsettings.h"
#include "rgbleds.h"
#endif

#if defined(PIOS_INCLUDE_DEBUG_CONSOLE) && defined(DEBUG_THIS_FILE)
#define DEBUG_MSG(format, ...) PIOS_COM_SendFormattedString(PIOS_COM_DEBUG, format, ## __VA_ARGS__)
#else
#define DEBUG_MSG(format, ...)
#endif

#ifndef IDLE_COUNTS_PER_SEC_AT_NO_LOAD
#define IDLE_COUNTS_PER_SEC_AT_NO_LOAD 995998	// calibrated by running tests/test_cpuload.c
// must be updated if the FreeRTOS or compiler
// optimisation options are changed.
#endif

#if defined(PIOS_SYSTEM_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_SYSTEM_STACK_SIZE
#else
#define STACK_SIZE_BYTES 1024
#endif

#define TASK_PRIORITY PIOS_THREAD_PRIO_NORMAL

/* When we're blinking morse code, this works out to 10.6 WPM.  It's also
 * nice and relatively prime to most other rates of things, so we don't get
 * bad beat frequencies. */

#ifdef SYSTEMMOD_RGBLED_SUPPORT
#define SYSTEM_RAPID_UPDATES

#define SYSTEM_UPDATE_PERIOD_MS 29
#define SYSTEM_UPDATE_PERIOD_MS4TH (SYSTEM_UPDATE_PERIOD_MS * 4)

#else

#define SYSTEM_UPDATE_PERIOD_MS 117
#define SYSTEM_UPDATE_PERIOD_MS4TH (SYSTEM_UPDATE_PERIOD_MS)

#endif

// Private types

/**
 * Event callback information
 */
typedef struct {
	UAVObjEvent ev; /** The actual event */
	UAVObjEventCallback cb; /** The callback function, or zero if none */
	struct pios_queue *queue; /** The queue or zero if none */
} EventCallbackInfo;

/**
 * List of object properties that are needed for the periodic updates.
 */
struct PeriodicObjectListStruct {
	EventCallbackInfo evInfo; /** Event callback information */
	uint16_t updatePeriodMs; /** Update period in ms or 0 if no periodic updates are needed */
	int32_t timeToNextUpdateMs; /** Time delay to the next update */
	struct PeriodicObjectListStruct* next; /** Needed by linked list library (utlist.h) */
};
typedef struct PeriodicObjectListStruct PeriodicObjectList;

// Private types

// Private variables
static PeriodicObjectList* objList;
static struct pios_recursive_mutex *mutex;
static EventStats stats;

static uint32_t idleCounter;
static uint32_t idleCounterClear;
static struct pios_thread *systemTaskHandle;
static struct pios_queue *objectPersistenceQueue;

static volatile bool config_check_needed;

// Private functions
static void systemPeriodicCb(UAVObjEvent *ev, void *ctx, void *obj_data, int len);
static void objectUpdatedCb(UAVObjEvent * ev, void *ctx, void *obj, int len);
static uint32_t processPeriodicUpdates();
static int32_t eventPeriodicCreate(UAVObjEvent* ev, UAVObjEventCallback cb, struct pios_queue *queue, uint16_t periodMs);
static int32_t eventPeriodicUpdate(UAVObjEvent* ev, UAVObjEventCallback cb, struct pios_queue *queue, uint16_t periodMs);

#ifndef NO_SENSORS
static void configurationUpdatedCb(UAVObjEvent * ev, void *ctx, void *obj, int len);
#endif

static void systemTask(void *parameters);
static inline void updateStats();
static inline void updateSystemAlarms();
static inline void updateRfm22bStats();
#if defined(WDG_STATS_DIAGNOSTICS)
static inline void updateWDGstats();
#endif

/**
 * Create the module task.
 * \returns 0 on success or -1 if initialization failed
 */
int32_t SystemModStart(void)
{
	// Create periodic event that will be used to update the telemetry stats
	UAVObjEvent ev;
	memset(&ev, 0, sizeof(UAVObjEvent));
	EventPeriodicCallbackCreate(&ev, systemPeriodicCb, SYSTEM_UPDATE_PERIOD_MS);

	EventClearStats();

	// Create system task
	systemTaskHandle = PIOS_Thread_Create(systemTask, "System", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
	// Register task
	TaskMonitorAdd(TASKINFO_RUNNING_SYSTEM, systemTaskHandle);

	return 0;
}

/**
 * Initialize the module, called on startup.
 * \returns 0 on success or -1 if initialization failed
 */
int32_t SystemModInitialize(void)
{
	// Create mutex
	mutex = PIOS_Recursive_Mutex_Create();
	if (mutex == NULL)
		return -1;

	if (SystemSettingsInitialize() == -1
			|| SystemStatsInitialize() == -1
			|| FlightStatusInitialize() == -1
			|| ObjectPersistenceInitialize() == -1
			|| AnnunciatorSettingsInitialize() == -1
#ifdef SYSTEMMOD_RGBLED_SUPPORT
			|| RGBLEDSettingsInitialize() == -1
#endif
	   )	{
		return -1;
	}
#if defined(DIAG_TASKS)
	if (TaskInfoInitialize() == -1)
		return -1;
#endif
#if defined(WDG_STATS_DIAGNOSTICS)
	if (WatchdogStatusInitialize() == -1)
		return -1;
#endif

	objectPersistenceQueue = PIOS_Queue_Create(1, sizeof(UAVObjEvent));
	if (objectPersistenceQueue == NULL)
		return -1;

	SystemModStart();

	return 0;
}

	MODULE_HIPRI_INITCALL(SystemModInitialize, 0)
static void systemTask(void *parameters)
{
	/* create all modules thread */
	MODULE_TASKCREATE_ALL;

	if (PIOS_heap_malloc_failed_p()) {
		/* We failed to malloc during task creation,
		 * system behaviour is undefined.  Reset and let
		 * the BootFault code recover for us.
		 */
		PIOS_SYS_Reset();
	}

#if defined(PIOS_INCLUDE_IAP)
	/* Record a successful boot */
	PIOS_IAP_WriteBootCount(0);
#endif

	// Initialize vars
	idleCounter = 0;
	idleCounterClear = 0;

	// Listen for SettingPersistance object updates, connect a callback function
	ObjectPersistenceConnectQueue(objectPersistenceQueue);

#ifndef NO_SENSORS
	// Run this initially to make sure the configuration is checked
	configuration_check();

	// Whenever the configuration changes, make sure it is safe to fly
	if (StabilizationSettingsHandle())
		StabilizationSettingsConnectCallback(configurationUpdatedCb);
	if (SystemSettingsHandle())
		SystemSettingsConnectCallback(configurationUpdatedCb);
	if (ManualControlSettingsHandle())
		ManualControlSettingsConnectCallback(configurationUpdatedCb);
	if (FlightStatusHandle())
		FlightStatusConnectCallback(configurationUpdatedCb);
#ifndef SMALLF1
	if (StateEstimationHandle())
		StateEstimationConnectCallback(configurationUpdatedCb);
#endif
#endif

	// Main system loop
	while (1) {
		int32_t delayTime = processPeriodicUpdates();

		UAVObjEvent ev;

		if (PIOS_Queue_Receive(objectPersistenceQueue, &ev, delayTime) == true) {
			// If object persistence is updated call the callback
			objectUpdatedCb(&ev, NULL, NULL, 0);
		}
	}
}

#if defined(PIOS_INCLUDE_ANNUNC)

#define BLINK_STRING_RADIO "r "

/**
 * Indicate there are conditions worth an error indication
 */
static inline uint8_t indicate_error(const char **sequence)
{
	SystemAlarmsData alarms;
	SystemAlarmsGet(&alarms);

	*sequence = NULL;

	uint8_t worst_sev = 0;

	bool generic = false;

	for (uint32_t i = 0; i < SYSTEMALARMS_ALARM_NUMELEM; i++) {
		// If this is less severe than the current alarm, continue.
		if (alarms.Alarm[i] < worst_sev) {
			continue;
		}

		// If there's a tie and the previous answer is not the
		// 'generic' alarm, continue.
		if ((!generic) && (alarms.Alarm[i] == worst_sev)) {
			continue;
		}

		uint8_t thresh = SYSTEMALARMS_ALARM_WARNING;

		if (i == SYSTEMALARMS_ALARM_TELEMETRY) {
			// Suppress most alarms from telemetry.
			// The user can identify if present from GCS.
			thresh = SYSTEMALARMS_ALARM_CRITICAL;
		}

		if (i == SYSTEMALARMS_ALARM_EVENTSYSTEM) {
			// Skip event system alarms-- nuisance
			continue;
		}

		if (alarms.Alarm[i] < thresh) {
			continue;
		}

		// OK, we have a candidate alarm.

		worst_sev = alarms.Alarm[i];
		generic = false;

		switch (i) {
			case SYSTEMALARMS_ALARM_BATTERY:
				// -...    b for battery
				*sequence = "b ";
				break;
			case SYSTEMALARMS_ALARM_SYSTEMCONFIGURATION:
				// -.-.    c for configuration
				*sequence = "c ";
				break;
			case SYSTEMALARMS_ALARM_GPS:
				// --.     g for GPS
				*sequence = "g ";
				break;
			case SYSTEMALARMS_ALARM_MANUALCONTROL:
				// .-.     r for radio
				*sequence = BLINK_STRING_RADIO;
				break;
			default:
				// .-      a for alarm
				*sequence = "a ";

				generic = true;
				break;
		}
	}

	return worst_sev;
}
#endif

DONT_BUILD_IF(ANNUNCIATORSETTINGS_ANNUNCIATEAFTERARMING_NUMELEM !=
	ANNUNCIATORSETTINGS_ANNUNCIATEANYTIME_NUMELEM,
	AnnuncSettingsMismatch1);
DONT_BUILD_IF(ANNUNCIATORSETTINGS_ANNUNCIATEAFTERARMING_MAXOPTVAL !=
	ANNUNCIATORSETTINGS_ANNUNCIATEANYTIME_MAXOPTVAL,
	AnnuncSettingsMismatch2);

#if defined(PIOS_INCLUDE_ANNUNC)
static inline bool should_annunc(AnnunciatorSettingsData *annunciatorSettings,
		bool been_armed, bool is_manual_control, uint8_t blink_prio,
		uint8_t cfg_field) {
	PIOS_Assert(cfg_field <
			ANNUNCIATORSETTINGS_ANNUNCIATEAFTERARMING_NUMELEM);

	if (been_armed) {
		if (blink_prio >= annunciatorSettings->AnnunciateAfterArming[cfg_field]) {
			return true;
		} else if (is_manual_control && annunciatorSettings->AnnunciateAfterArming[cfg_field] == ANNUNCIATORSETTINGS_ANNUNCIATEAFTERARMING_MANUALCONTROLONLY) {
			return true;
		}
	}

	if (blink_prio >= annunciatorSettings->AnnunciateAnytime[cfg_field]) {
		return true;
	} else if (is_manual_control && annunciatorSettings->AnnunciateAnytime[cfg_field] == ANNUNCIATORSETTINGS_ANNUNCIATEANYTIME_MANUALCONTROLONLY) {
		return true;
	}

	return false;
}

static inline void consider_annunc(AnnunciatorSettingsData *annunciatorSettings,
		bool is_active, bool been_armed, bool is_manual_control,
		uint8_t blink_prio, uint32_t annunc_id,
		uint8_t cfg_field) {
	if (
			is_active &&
			should_annunc(annunciatorSettings, been_armed,
				is_manual_control, blink_prio, cfg_field)
	   ) {
		PIOS_ANNUNC_On(annunc_id);
	} else {
		PIOS_ANNUNC_Off(annunc_id);
	}
}
#endif

static void systemPeriodicCb(UAVObjEvent *ev, void *ctx, void *obj_data, int len) {
	(void) ev; (void) ctx; (void) obj_data; (void) len;

	static unsigned int counter=0;

	counter++;

#ifndef NO_SENSORS
	if (config_check_needed) {
		configuration_check();
		config_check_needed = false;
	}
#endif

#ifdef SYSTEM_RAPID_UPDATES
	bool fourth = (counter & 3) == 0;
#else
	bool fourth = true;	// All of this stuff, do each time
#endif

	if (fourth) {
		// Update the modem status, if present
		updateRfm22bStats();

#ifndef PIPXTREME
		// Update the system statistics
		updateStats();

		// Update the system alarms
		updateSystemAlarms();

#if defined(WDG_STATS_DIAGNOSTICS)
		updateWDGstats();
#endif

#if defined(DIAG_TASKS)
		// Update the task status object
		TaskMonitorUpdateAll();
#endif

#endif /* PIPXTREME */
	}

#if !defined(PIPXTREME) && defined(PIOS_INCLUDE_ANNUNC)
	// Figure out what we should be doing.

	static const char *blink_string = NULL;
	static uint32_t blink_state = 0;
	static uint8_t blink_prio = 0;
	static bool ever_armed = false;
	static uint8_t armed_status = FLIGHTSTATUS_ARMED_DISARMED;
	static bool is_manual_control = false;
	static int morse;

#ifdef SYSTEMMOD_RGBLED_SUPPORT
	static bool led_override = false;
#endif

	// Evaluate all our possible annunciator sources.

	// The most important: indicate_error / alarms

	if (fourth) {
		const char *candidate_blink;
		uint8_t candidate_prio;

		candidate_prio = indicate_error(&candidate_blink);

		if (candidate_prio > blink_prio) {
			// Preempt!
			blink_state = 0;
			blink_string = candidate_blink;
			blink_prio = candidate_prio;

			if (blink_string &&
					!strcmp(blink_string, BLINK_STRING_RADIO)) {
				// XXX do this in a more robust way */
				is_manual_control = true;
			} else {
				is_manual_control = false;
			}
		}

		FlightStatusArmedGet(&armed_status);

		if (armed_status == FLIGHTSTATUS_ARMED_ARMED) {
			ever_armed = true;
		}

		if ((blink_prio == 0) && (blink_state == 0)) {
			// Nothing else to do-- show armed status
			if (armed_status == FLIGHTSTATUS_ARMED_ARMED) {
				blink_string = "I";	// .. pairs of blinks.
			} else {
				blink_string = "T";	// - single long blinks
			}

			blink_prio = SHAREDDEFS_ALARMLEVELS_OK;
		}

		morse = morse_send(&blink_string, &blink_state);

		AnnunciatorSettingsData annunciatorSettings;
		AnnunciatorSettingsGet(&annunciatorSettings);

#ifdef PIOS_LED_HEARTBEAT
		consider_annunc(&annunciatorSettings, morse > 0, ever_armed,
				is_manual_control,
				blink_prio, PIOS_LED_HEARTBEAT,
				ANNUNCIATORSETTINGS_ANNUNCIATEANYTIME_LED_HEARTBEAT);
#endif

#ifdef PIOS_LED_ALARM
		consider_annunc(&annunciatorSettings, morse > 0, ever_armed,
				is_manual_control,
				blink_prio, PIOS_LED_ALARM,
				ANNUNCIATORSETTINGS_ANNUNCIATEANYTIME_LED_ALARM);
#endif

#ifdef PIOS_ANNUNCIATOR_BUZZER
		consider_annunc(&annunciatorSettings, morse > 0, ever_armed,
				is_manual_control,
				blink_prio, PIOS_ANNUNCIATOR_BUZZER,
				ANNUNCIATORSETTINGS_ANNUNCIATEANYTIME_BUZZER);
#endif

#ifdef SYSTEMMOD_RGBLED_SUPPORT
		led_override = should_annunc(&annunciatorSettings,
				ever_armed, is_manual_control, blink_prio,
				ANNUNCIATORSETTINGS_ANNUNCIATEANYTIME_RGB_LEDS);
#endif

		if (morse < 0) {
			// This means we were told "completed"
			blink_string = NULL;
			blink_prio = 0;

			is_manual_control = false;
		}
	}

#ifdef SYSTEMMOD_RGBLED_SUPPORT
	systemmod_process_rgb_leds(led_override, morse > 0, blink_prio,
		FLIGHTSTATUS_ARMED_DISARMED != armed_status,
		(FLIGHTSTATUS_ARMED_ARMING == armed_status) &&
		((counter & 3) < 2));
#endif /* SYSTEMMOD_RGBLED_SUPPORT */

#endif  /* !PIPXTREME && PIOS_INCLUDE_ANNUNC */
}


/**
 * Function called in response to object updates
 */
static void objectUpdatedCb(UAVObjEvent * ev, void *ctx, void *obj_data, int len)
{
	(void) ctx; (void) obj_data; (void) len;

	/* Handled in RadioComBridge on pipxtreme. */
#ifndef PIPXTREME
	ObjectPersistenceData objper;
	UAVObjHandle obj;

	// If the object updated was the ObjectPersistence execute requested action
	if (ev->obj == ObjectPersistenceHandle()) {
		// Get object data
		ObjectPersistenceGet(&objper);

		int retval = 1;

		// When this is called because of this method don't do anything
		if (objper.Operation == OBJECTPERSISTENCE_OPERATION_ERROR ||
				objper.Operation == OBJECTPERSISTENCE_OPERATION_COMPLETED) {
			return;
		}

		if (objper.Operation == OBJECTPERSISTENCE_OPERATION_LOAD) {
			// Get selected object
			obj = UAVObjGetByID(objper.ObjectID);
			if (obj == 0) {
				return;
			}
			// Load selected instance
			retval = UAVObjLoad(obj, objper.InstanceID);
		} else if (objper.Operation == OBJECTPERSISTENCE_OPERATION_SAVE) {
			// Get selected object
			obj = UAVObjGetByID(objper.ObjectID);
			if (obj == 0) {
				return;
			}

			// Save selected instance
			retval = UAVObjSave(obj, objper.InstanceID);
		} else if (objper.Operation == OBJECTPERSISTENCE_OPERATION_DELETE) {
			// Delete selected instance
			retval = UAVObjDeleteById(objper.ObjectID, objper.InstanceID);
		} else if (objper.Operation == OBJECTPERSISTENCE_OPERATION_FULLERASE) {
			retval = -1;
#if defined(PIOS_INCLUDE_LOGFS_SETTINGS)
			extern uintptr_t pios_uavo_settings_fs_id;
			retval = PIOS_FLASHFS_Format(pios_uavo_settings_fs_id);
#endif
		}

		// Yield when saving, so if there's a ton of updates we don't
		// prevent other threads from updatin'.
		PIOS_Thread_Sleep(25);

		switch(retval) {
			case 0:
				objper.Operation = OBJECTPERSISTENCE_OPERATION_COMPLETED;
				ObjectPersistenceSet(&objper);
				break;
			case -1:
				objper.Operation = OBJECTPERSISTENCE_OPERATION_ERROR;
				ObjectPersistenceSet(&objper);
				break;
			default:
				break;
		}
	}
#endif
}

#ifndef NO_SENSORS
/**
 * Called whenever a critical configuration component changes
 */

static void configurationUpdatedCb(UAVObjEvent * ev, void *ctx, void *obj, int len)
{
	(void) ev; (void) ctx; (void) obj; (void) len;
	config_check_needed = true;
}
#endif

/**
 * Called periodically to update the WDG statistics
 */
#if defined(WDG_STATS_DIAGNOSTICS)
static WatchdogStatusData watchdogStatus;
static void updateWDGstats()
{
	// Only update if something has changed
	if (watchdogStatus.ActiveFlags != PIOS_WDG_GetActiveFlags() ||
			watchdogStatus.BootupFlags != PIOS_WDG_GetBootupFlags()) {
		watchdogStatus.BootupFlags = PIOS_WDG_GetBootupFlags();
		watchdogStatus.ActiveFlags = PIOS_WDG_GetActiveFlags();
		WatchdogStatusSet(&watchdogStatus);
	}
}
#endif

#ifdef PIPXTREME
#define RFM22BSTATUSINST 0
#else
#define RFM22BSTATUSINST 1
#endif

static void updateRfm22bStats() {
#if defined(PIOS_INCLUDE_RFM22B)
	if (pios_rfm22b_id) {

		// Update the RFM22BStatus UAVO
		RFM22BStatusData rfm22bStatus;
		RFM22BStatusInstGet(RFM22BSTATUSINST, &rfm22bStatus);

		// Get the stats from the radio device
		struct rfm22b_stats radio_stats;
		PIOS_RFM22B_GetStats(pios_rfm22b_id, &radio_stats);

		// Update the LInk status
		static bool first_time = true;
		static uint16_t prev_tx_count = 0;
		static uint16_t prev_rx_count = 0;
		rfm22bStatus.HeapRemaining = PIOS_heap_get_free_size();
		rfm22bStatus.RxGood = radio_stats.rx_good;
		rfm22bStatus.RxCorrected   = radio_stats.rx_corrected;
		rfm22bStatus.RxErrors = radio_stats.rx_error;
		rfm22bStatus.RxSyncMissed = radio_stats.rx_sync_missed;
		rfm22bStatus.TxMissed = radio_stats.tx_missed;
		rfm22bStatus.RxFailure     = radio_stats.rx_failure;
		rfm22bStatus.Resets      = radio_stats.resets;
		rfm22bStatus.Timeouts    = radio_stats.timeouts;
		rfm22bStatus.RSSI        = radio_stats.rssi;
		rfm22bStatus.LinkQuality = radio_stats.link_quality;
		if (first_time) {
			first_time = false;
		} else {
			uint16_t tx_count = radio_stats.tx_byte_count;
			uint16_t rx_count = radio_stats.rx_byte_count;
			uint16_t tx_bytes = (tx_count < prev_tx_count) ? (0xffff - prev_tx_count + tx_count) : (tx_count - prev_tx_count);
			uint16_t rx_bytes = (rx_count < prev_rx_count) ? (0xffff - prev_rx_count + rx_count) : (rx_count - prev_rx_count);
			rfm22bStatus.TXRate = (uint16_t)((float)(tx_bytes * 1000) / (SYSTEM_UPDATE_PERIOD_MS4TH));
			rfm22bStatus.RXRate = (uint16_t)((float)(rx_bytes * 1000) / (SYSTEM_UPDATE_PERIOD_MS4TH));
			prev_tx_count = tx_count;
			prev_rx_count = rx_count;
		}

		rfm22bStatus.LinkState = radio_stats.link_state;
		RFM22BStatusInstSet(RFM22BSTATUSINST, &rfm22bStatus);
	}
#endif /* if defined(PIOS_INCLUDE_RFM22B) */
}

/**
 * Called periodically to update the system stats
 */
static uint16_t GetFreeIrqStackSize(void)
{
	uint32_t i = 0x200;

#if !defined(ARCH_POSIX) && !defined(ARCH_WIN32) && defined(CHECK_IRQ_STACK)
	extern uint32_t _irq_stack_top;
	extern uint32_t _irq_stack_end;
	uint32_t pattern = 0x0000A5A5;
	uint32_t *ptr = &_irq_stack_end;

	uint32_t stack_size = (((uint32_t)&_irq_stack_top - (uint32_t)&_irq_stack_end) & ~3 ) / 4;

	for (i=0; i< stack_size; i++)
	{
		if (ptr[i] != pattern)
		{
			i=i*4;
			break;
		}
	}
#endif
	return i;
}

/**
 * Called periodically to update the system stats
 */
static void updateStats()
{
	static uint32_t lastTickCount = 0;
	SystemStatsData stats;

	// Get stats and update
	SystemStatsGet(&stats);
	stats.FlightTime = PIOS_Thread_Systime();
	stats.HeapRemaining = PIOS_heap_get_free_size();
	stats.FastHeapRemaining = PIOS_fastheap_get_free_size();

	// Get Irq stack status
	stats.IRQStackRemaining = GetFreeIrqStackSize();

	// When idleCounterClear was not reset by the idle-task, it means the idle-task did not run
	if (idleCounterClear) {
		idleCounter = 0;
	}

	uint32_t now = PIOS_Thread_Systime();
	if (now > lastTickCount) {
		float dT = (PIOS_Thread_Systime() - lastTickCount) / 1000.0f;

		// In the case of a slightly miscalibrated max idle count, make sure CPULoad does
		// not go negative and set an alarm inappropriately.
		float idleFraction = ((float)idleCounter / dT) / (float)IDLE_COUNTS_PER_SEC_AT_NO_LOAD;
		if (idleFraction > 1)
			stats.CPULoad = 0;
		else
			stats.CPULoad = 100 - roundf(100.0f * idleFraction);
	} //else: TickCount has wrapped, do not calc now

	lastTickCount = now;
	idleCounterClear = 1;

#if defined(PIOS_INCLUDE_ADC) && defined(PIOS_ADC_USE_TEMP_SENSOR)
	float temp_voltage = 3.3f * PIOS_ADC_DevicePinGet(PIOS_INTERNAL_ADC, 0) / ((1 << 12) - 1);
	const float STM32_TEMP_V25 = 1.43f; /* V */
	const float STM32_TEMP_AVG_SLOPE = 4.3f; /* mV/C */
	stats.CPUTemp = (temp_voltage-STM32_TEMP_V25) * 1000 / STM32_TEMP_AVG_SLOPE + 25;
#endif
	SystemStatsSet(&stats);
}

/**
 * Update system alarms
 */
static void updateSystemAlarms()
{
#ifndef PIPXTREME
	SystemStatsData stats;
	UAVObjStats objStats;
	EventStats evStats;
	SystemStatsGet(&stats);

	// Check heap, IRQ stack and malloc failures
	if (PIOS_heap_malloc_failed_p()
			|| (stats.HeapRemaining < HEAP_LIMIT_CRITICAL)
#if !defined(ARCH_POSIX) && !defined(ARCH_WIN32) && defined(CHECK_IRQ_STACK)
			|| (stats.IRQStackRemaining < IRQSTACK_LIMIT_CRITICAL)
#endif
	   ) {
		AlarmsSet(SYSTEMALARMS_ALARM_OUTOFMEMORY, SYSTEMALARMS_ALARM_CRITICAL);
	} else if (
			(stats.HeapRemaining < HEAP_LIMIT_WARNING)
#if !defined(ARCH_POSIX) && !defined(ARCH_WIN32) && defined(CHECK_IRQ_STACK)
			|| (stats.IRQStackRemaining < IRQSTACK_LIMIT_WARNING)
#endif
		  ) {
		AlarmsSet(SYSTEMALARMS_ALARM_OUTOFMEMORY, SYSTEMALARMS_ALARM_WARNING);
	} else {
		AlarmsClear(SYSTEMALARMS_ALARM_OUTOFMEMORY);
	}

	// Check CPU load
#ifdef SIM_POSIX
	// XXX do something meaningful here in the future.
	// Right now CPU monitoring on simulator is worthless.
	AlarmsClear(SYSTEMALARMS_ALARM_CPUOVERLOAD);
#else
	if (stats.CPULoad > CPULOAD_LIMIT_CRITICAL) {
		AlarmsSet(SYSTEMALARMS_ALARM_CPUOVERLOAD, SYSTEMALARMS_ALARM_CRITICAL);
	} else if (stats.CPULoad > CPULOAD_LIMIT_WARNING) {
		AlarmsSet(SYSTEMALARMS_ALARM_CPUOVERLOAD, SYSTEMALARMS_ALARM_WARNING);
	} else {
		AlarmsClear(SYSTEMALARMS_ALARM_CPUOVERLOAD);
	}
#endif

	// Check for event errors
	UAVObjGetStats(&objStats);
	EventGetStats(&evStats);
	UAVObjClearStats();
	EventClearStats();
	if (objStats.eventCallbackErrors > 0 || objStats.eventQueueErrors > 0  || evStats.eventErrors > 0) {
		AlarmsSet(SYSTEMALARMS_ALARM_EVENTSYSTEM, SYSTEMALARMS_ALARM_WARNING);
	} else {
		AlarmsClear(SYSTEMALARMS_ALARM_EVENTSYSTEM);
	}

	if (objStats.lastCallbackErrorID || objStats.lastQueueErrorID || evStats.lastErrorID) {
		SystemStatsData sysStats;
		SystemStatsGet(&sysStats);
		sysStats.EventSystemWarningID = evStats.lastErrorID;
		sysStats.ObjectManagerCallbackID = objStats.lastCallbackErrorID;
		sysStats.ObjectManagerQueueID = objStats.lastQueueErrorID;
		SystemStatsSet(&sysStats);
	}
#endif
}

/**
 * Called by the RTOS when the CPU is idle, used to measure the CPU idle time.
 */
void vApplicationIdleHook(void)
{
	// Called when the scheduler has no tasks to run
	if (idleCounterClear == 0) {
		++idleCounter;
	} else {
		idleCounter = 0;
		idleCounterClear = 0;
	}
}

/**
 * Get the statistics counters
 * @param[out] statsOut The statistics counters will be copied there
 */
void EventGetStats(EventStats* statsOut)
{
	PIOS_Recursive_Mutex_Lock(mutex, PIOS_MUTEX_TIMEOUT_MAX);
	memcpy(statsOut, &stats, sizeof(EventStats));
	PIOS_Recursive_Mutex_Unlock(mutex);
}

/**
 * Clear the statistics counters
 */
void EventClearStats()
{
	PIOS_Recursive_Mutex_Lock(mutex, PIOS_MUTEX_TIMEOUT_MAX);
	memset(&stats, 0, sizeof(EventStats));
	PIOS_Recursive_Mutex_Unlock(mutex);
}

/**
 * Dispatch an event at periodic intervals.
 * \param[in] ev The event to be dispatched
 * \param[in] cb The callback to be invoked
 * \param[in] periodMs The period the event is generated
 * \return Success (0), failure (-1)
 */
/* XXX TODO: would be nice to get context record logic in these */
int32_t EventPeriodicCallbackCreate(UAVObjEvent* ev, UAVObjEventCallback cb, uint16_t periodMs)
{
	return eventPeriodicCreate(ev, cb, 0, periodMs);
}

/**
 * Update the period of a periodic event.
 * \param[in] ev The event to be dispatched
 * \param[in] cb The callback to be invoked
 * \param[in] periodMs The period the event is generated
 * \return Success (0), failure (-1)
 */
int32_t EventPeriodicCallbackUpdate(UAVObjEvent* ev, UAVObjEventCallback cb, uint16_t periodMs)
{
	return eventPeriodicUpdate(ev, cb, 0, periodMs);
}

/**
 * Dispatch an event at periodic intervals.
 * \param[in] ev The event to be dispatched
 * \param[in] queue The queue that the event will be pushed in
 * \param[in] periodMs The period the event is generated
 * \return Success (0), failure (-1)
 */
int32_t EventPeriodicQueueCreate(UAVObjEvent* ev, struct pios_queue *queue, uint16_t periodMs)
{
	return eventPeriodicCreate(ev, 0, queue, periodMs);
}

/**
 * Update the period of a periodic event.
 * \param[in] ev The event to be dispatched
 * \param[in] queue The queue
 * \param[in] periodMs The period the event is generated
 * \return Success (0), failure (-1)
 */
int32_t EventPeriodicQueueUpdate(UAVObjEvent* ev, struct pios_queue *queue, uint16_t periodMs)
{
	return eventPeriodicUpdate(ev, 0, queue, periodMs);
}

/**
 * Dispatch an event through a callback at periodic intervals.
 * \param[in] ev The event to be dispatched
 * \param[in] cb The callback to be invoked or zero if none
 * \param[in] queue The queue or zero if none
 * \param[in] periodMs The period the event is generated
 * \return Success (0), failure (-1)
 */
static int32_t eventPeriodicCreate(UAVObjEvent* ev, UAVObjEventCallback cb, struct pios_queue *queue, uint16_t periodMs)
{
	PeriodicObjectList* objEntry;
	// Get lock
	PIOS_Recursive_Mutex_Lock(mutex, PIOS_MUTEX_TIMEOUT_MAX);
	// Check that the object is not already connected
	LL_FOREACH(objList, objEntry)
	{
		if (objEntry->evInfo.cb == cb &&
				objEntry->evInfo.queue == queue &&
				objEntry->evInfo.ev.obj == ev->obj &&
				objEntry->evInfo.ev.instId == ev->instId &&
				objEntry->evInfo.ev.event == ev->event)
		{
			// Already registered, do nothing
			PIOS_Recursive_Mutex_Unlock(mutex);
			return -1;
		}
	}
	// Create handle
	objEntry = (PeriodicObjectList*)PIOS_malloc_no_dma(sizeof(PeriodicObjectList));
	if (objEntry == NULL) return -1;
	objEntry->evInfo.ev.obj = ev->obj;
	objEntry->evInfo.ev.instId = ev->instId;
	objEntry->evInfo.ev.event = ev->event;
	objEntry->evInfo.cb = cb;
	objEntry->evInfo.queue = queue;
	objEntry->updatePeriodMs = periodMs;
	objEntry->timeToNextUpdateMs = randomize_int(periodMs); // avoid bunching of updates
	// Add to list
	LL_APPEND(objList, objEntry);
	// Release lock
	PIOS_Recursive_Mutex_Unlock(mutex);
	return 0;
}

/**
 * Update the period of a periodic event.
 * \param[in] ev The event to be dispatched
 * \param[in] cb The callback to be invoked or zero if none
 * \param[in] queue The queue or zero if none
 * \param[in] periodMs The period the event is generated
 * \return Success (0), failure (-1)
 */
static int32_t eventPeriodicUpdate(UAVObjEvent* ev, UAVObjEventCallback cb, struct pios_queue *queue, uint16_t periodMs)
{
	PeriodicObjectList* objEntry;
	// Get lock
	PIOS_Recursive_Mutex_Lock(mutex, PIOS_MUTEX_TIMEOUT_MAX);
	// Find object
	LL_FOREACH(objList, objEntry)
	{
		if (objEntry->evInfo.cb == cb &&
				objEntry->evInfo.queue == queue &&
				objEntry->evInfo.ev.obj == ev->obj &&
				objEntry->evInfo.ev.instId == ev->instId &&
				objEntry->evInfo.ev.event == ev->event)
		{
			// Object found, update period
			objEntry->updatePeriodMs = periodMs;
			objEntry->timeToNextUpdateMs = randomize_int(periodMs); // avoid bunching of updates
			// Release lock
			PIOS_Recursive_Mutex_Unlock(mutex);
			return 0;
		}
	}
	// If this point is reached the object was not found
	PIOS_Recursive_Mutex_Unlock(mutex);
	return -1;
}

/* It can take this long before a "first callback" on a registration,
 * so it's advantageous for it to not be too long.  (e.g. we don't have a
 * mechanism to wakeup on list change */
#define MAX_UPDATE_PERIOD_MS 350

/**
 * Handle periodic updates for all objects.
 * \return The system time until the next update (in ms) or -1 if failed
 */
static uint32_t processPeriodicUpdates()
{
	PeriodicObjectList* objEntry;
	int32_t timeNow;
	int32_t offset;

	// Get lock
	PIOS_Recursive_Mutex_Lock(mutex, PIOS_MUTEX_TIMEOUT_MAX);

	// Iterate through each object and update its timer, if zero then transmit object.
	// Also calculate smallest delay to next update.
	uint32_t now = PIOS_Thread_Systime();
	uint32_t timeToNextUpdate = PIOS_Thread_Systime() + MAX_UPDATE_PERIOD_MS;
	LL_FOREACH(objList, objEntry)
	{
		// If object is configured for periodic updates
		if (objEntry->updatePeriodMs > 0)
		{
			// Check if time for the next update
			timeNow = PIOS_Thread_Systime();
			if (objEntry->timeToNextUpdateMs <= timeNow)
			{
				// Reset timer
				offset = ( timeNow - objEntry->timeToNextUpdateMs ) % objEntry->updatePeriodMs;
				objEntry->timeToNextUpdateMs = timeNow + objEntry->updatePeriodMs - offset;
				// Invoke callback, if one
				if ( objEntry->evInfo.cb != 0)
				{
					objEntry->evInfo.cb(&objEntry->evInfo.ev, NULL, NULL, 0); // the function is expected to copy the event information
				}
				// Push event to queue, if one
				if ( objEntry->evInfo.queue != 0)
				{
					if (PIOS_Queue_Send(objEntry->evInfo.queue, &objEntry->evInfo.ev, 0) != true ) // do not block if queue is full
					{
						if (objEntry->evInfo.ev.obj != NULL)
							stats.lastErrorID = UAVObjGetID(objEntry->evInfo.ev.obj);
						++stats.eventErrors;
					}
				}
			}
			// Update minimum delay
			if (objEntry->timeToNextUpdateMs < timeToNextUpdate)
			{
				timeToNextUpdate = objEntry->timeToNextUpdateMs;
			}
		}
	}

	// Done
	PIOS_Recursive_Mutex_Unlock(mutex);
	return timeToNextUpdate - now;
}

/**
 * @}
 * @}
 */
