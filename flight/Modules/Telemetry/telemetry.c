/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup TelemetryModule Telemetry Module
 * @{
 *
 * @file       telemetry.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @brief      Telemetry module, handles telemetry and UAVObject updates
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
#include <eventdispatcher.h>
#include "flighttelemetrystats.h"
#include "gcstelemetrystats.h"
#include "modulesettings.h"
#include "sessionmanaging.h"
#include "pios_thread.h"
#include "pios_queue.h"

#include "pios_hal.h"

#include <uavtalk.h>

// Private constants
#define MAX_QUEUE_SIZE   TELEM_QUEUE_SIZE
#define STACK_SIZE_BYTES PIOS_TELEM_STACK_SIZE
#define TASK_PRIORITY_RX PIOS_THREAD_PRIO_NORMAL
#define TASK_PRIORITY_TX PIOS_THREAD_PRIO_NORMAL
#define REQ_TIMEOUT_MS 250
#define MAX_RETRIES 2
#define STATS_UPDATE_PERIOD_MS 4000
#define CONNECTION_TIMEOUT_MS 8000
#define USB_ACTIVITY_TIMEOUT_MS 6000

// Private types

// Private variables
static struct pios_queue *queue;

static uint32_t txErrors;
static uint32_t txRetries;
static uint32_t timeOfLastObjectUpdate;
static UAVTalkConnection uavTalkCon;

#if defined(PIOS_INCLUDE_USB)
static volatile uint32_t usb_timeout_time;
#endif

// Private functions
static void telemetryTxTask(void *parameters);
static void telemetryRxTask(void *parameters);
static int32_t transmitData(uint8_t * data, int32_t length);
static void registerObject(UAVObjHandle obj);
static void updateObject(UAVObjHandle obj, int32_t eventType);
static int32_t setUpdatePeriod(UAVObjHandle obj, int32_t updatePeriodMs);
static void processObjEvent(UAVObjEvent * ev);
static void updateTelemetryStats();
static void gcsTelemetryStatsUpdated();
static void updateSettings();
static uintptr_t getComPort();
static void session_managing_updated(UAVObjEvent * ev, void *ctx, void *obj,
		int len);
static void update_object_instances(uint32_t obj_id, uint32_t inst_id);

/**
 * Initialise the telemetry module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t TelemetryStart(void)
{
	// Process all registered objects and connect queue for updates
	UAVObjIterate(&registerObject);

	// Create periodic event that will be used to update the telemetry stats
	UAVObjEvent ev;
	memset(&ev, 0, sizeof(UAVObjEvent));

	// Listen to objects of interest
	GCSTelemetryStatsConnectQueue(queue);
    
	struct pios_thread *telemetryTxTaskHandle;
	struct pios_thread *telemetryRxTaskHandle;

	// Start telemetry tasks
	telemetryTxTaskHandle = PIOS_Thread_Create(telemetryTxTask, "TelTx", STACK_SIZE_BYTES, NULL, TASK_PRIORITY_TX);
	telemetryRxTaskHandle = PIOS_Thread_Create(telemetryRxTask, "TelRx", STACK_SIZE_BYTES, NULL, TASK_PRIORITY_RX);
	TaskMonitorAdd(TASKINFO_RUNNING_TELEMETRYTX, telemetryTxTaskHandle);
	TaskMonitorAdd(TASKINFO_RUNNING_TELEMETRYRX, telemetryRxTaskHandle);

	return 0;
}

/**
 * Initialise the telemetry module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t TelemetryInitialize(void)
{
	if (FlightTelemetryStatsInitialize() == -1 || GCSTelemetryStatsInitialize() == -1) {
		return -1;
	}

	// Initialize vars
	timeOfLastObjectUpdate = 0;

	// Create object queues
	queue = PIOS_Queue_Create(MAX_QUEUE_SIZE, sizeof(UAVObjEvent));

	// Initialise UAVTalk
	uavTalkCon = UAVTalkInitialize(&transmitData);

	if (SessionManagingInitialize() == -1) {
		return -1;
	}
	
	SessionManagingConnectCallback(session_managing_updated);

	//register the new uavo instance callback function in the uavobjectmanager
	UAVObjRegisterNewInstanceCB(update_object_instances);

	return 0;
}

MODULE_HIPRI_INITCALL(TelemetryInitialize, TelemetryStart)

/**
 * Register a new object, adds object to local list and connects the queue depending on the object's
 * telemetry settings.
 * \param[in] obj Object to connect
 */
static void registerObject(UAVObjHandle obj)
{
	if (UAVObjIsMetaobject(obj)) {
		/* Only connect change notifications for meta objects.  No periodic updates */
		UAVObjConnectQueue(obj, queue, EV_MASK_ALL_UPDATES);
		return;
	} else {
		UAVObjMetadata metadata;
		UAVObjUpdateMode updateMode;
		UAVObjGetMetadata(obj, &metadata);
		updateMode = UAVObjGetTelemetryUpdateMode(&metadata);

		// Setup object for periodic updates
		UAVObjEvent ev = {
			.obj    = obj,
			.instId = UAVOBJ_ALL_INSTANCES,
			.event  = EV_UPDATED_PERIODIC,
		};

		/* Only create a periodic event for objects that are periodic */
		if (updateMode == UPDATEMODE_PERIODIC) {
			EventPeriodicQueueCreate(&ev, queue, 0);
		}

		// Setup object for telemetry updates
		updateObject(obj, EV_NONE);
	}
}

/**
 * Update object's queue connections and timer, depending on object's settings
 * \param[in] obj Object to updates
 */
static void updateObject(UAVObjHandle obj, int32_t eventType)
{
	UAVObjMetadata metadata;
	UAVObjUpdateMode updateMode;
	int32_t eventMask;

	if (UAVObjIsMetaobject(obj)) {
		/* This function updates the periodic updates for the object.
		 * Meta Objects cannot have periodic updates.
		 */
		PIOS_Assert(false);
		return;
	}

	// Get metadata
	UAVObjGetMetadata(obj, &metadata);
	updateMode = UAVObjGetTelemetryUpdateMode(&metadata);

	// Setup object depending on update mode
	switch (updateMode) {
	case UPDATEMODE_PERIODIC:
		// Set update period
		setUpdatePeriod(obj, metadata.telemetryUpdatePeriod);

		// Connect queue
		eventMask = EV_UPDATED_PERIODIC | EV_UPDATED_MANUAL;
		UAVObjConnectQueueThrottled(obj, queue, eventMask, 0);
		break;
	case UPDATEMODE_ONCHANGE:
		// Set update period
		setUpdatePeriod(obj, 0);

		// Connect queue
		eventMask = EV_UPDATED | EV_UPDATED_MANUAL;
		UAVObjConnectQueueThrottled(obj, queue, eventMask, 0);
		break;
	case UPDATEMODE_THROTTLED:
		setUpdatePeriod(obj, 0);

		eventMask = EV_UPDATED | EV_UPDATED_MANUAL;
		UAVObjConnectQueueThrottled(obj, queue, eventMask,
				metadata.telemetryUpdatePeriod);
		break;
	case UPDATEMODE_MANUAL:
		// Set update period
		setUpdatePeriod(obj, 0);

		// Connect queue
		eventMask = EV_UPDATED_MANUAL;
		UAVObjConnectQueueThrottled(obj, queue, eventMask, 0);
		break;
	}
}

/**
 * Processes queue events
 */
static void processObjEvent(UAVObjEvent * ev)
{
	UAVObjMetadata metadata;
	FlightTelemetryStatsData flightStats;
	int32_t retries;
	int32_t success;

	if (ev->obj == 0) {
		updateTelemetryStats();
	} else if (ev->obj == GCSTelemetryStatsHandle()) {
		gcsTelemetryStatsUpdated();
	} else {
		FlightTelemetryStatsGet(&flightStats);
		// Get object metadata
		UAVObjGetMetadata(ev->obj, &metadata);

		// Act on event
		retries = 0;
		success = -1;
		if (ev->event == EV_UPDATED || ev->event == EV_UPDATED_MANUAL ||
				ev->event == EV_UPDATED_PERIODIC) {
			// Send update to GCS (with retries)
			while (retries < MAX_RETRIES && success == -1) {
				success = UAVTalkSendObject(uavTalkCon, ev->obj, ev->instId, UAVObjGetTelemetryAcked(&metadata), REQ_TIMEOUT_MS);	// call blocks until ack is received or timeout

				++retries;
			}
			// Update stats
			txRetries += (retries - 1);
			if (success == -1) {
				++txErrors;
			}
		} 

		// If this is a metaobject then make necessary telemetry updates
		if (UAVObjIsMetaobject(ev->obj)) {
			updateObject(UAVObjGetLinkedObj(ev->obj), EV_NONE);     // linked object will be the actual object the metadata are for
		}
	}
}

/**
 * Telemetry transmit task, regular priority
 */
static void telemetryTxTask(void *parameters)
{
	// Update telemetry settings
	updateSettings();

	UAVObjEvent ev;

	// Loop forever
	while (1) {
		// Wait for queue message
		if (PIOS_Queue_Receive(queue, &ev, PIOS_QUEUE_TIMEOUT_MAX) == true) {
			// Process event
			processObjEvent(&ev);
		}
	}
}

#if defined(PIOS_INCLUDE_USB)
/**
 * Updates the USB activity timer, and returns whether we should use USB this
 * time around.
 * \param[in] seen_active true if we have seen activity on the USB port this time
 * \return true if we should use usb, false if not.
 */
static bool processUsbActivity(bool seen_active)
{
	uint32_t this_systime = PIOS_Thread_Systime();
	uint32_t fixedup_time = this_systime +
		USB_ACTIVITY_TIMEOUT_MS;

	if (seen_active) {
		if (fixedup_time < this_systime) {
			// (mostly) handle wrap.
			usb_timeout_time = UINT32_MAX;
		} else {
			usb_timeout_time = fixedup_time;
		}

		return true;
	}

	uint32_t usb_time = usb_timeout_time;

	if (this_systime >= usb_time) {
		usb_timeout_time = 0;

		return false;
	}

	/* If the timeout is too far in the future ...
	 * (because of the above wrap case..)  */
	if (fixedup_time < usb_time) {
		if (fixedup_time > this_systime) {
			/* and we're not wrapped, then fixup the
			 * time. */
			usb_timeout_time = fixedup_time;
		}
	}

	return true;
}
#endif // PIOS_INCLUDE_USB

/**
 * Telemetry transmit task. Processes queue events and periodic updates.
 */
static void telemetryRxTask(void *parameters)
{
	// Task loop
	while (1) {
		uintptr_t inputPort = getComPort();

		if (inputPort) {
			// Block until data are available
			uint8_t serial_data[16];
			uint16_t bytes_to_process;

			bytes_to_process = PIOS_COM_ReceiveBuffer(inputPort, serial_data, sizeof(serial_data), 500);
			if (bytes_to_process > 0) {
				for (uint8_t i = 0; i < bytes_to_process; i++) {
					UAVTalkProcessInputStream(uavTalkCon,serial_data[i]);
				}

#if defined(PIOS_INCLUDE_USB)
				if (inputPort == PIOS_COM_TELEM_USB) {
					processUsbActivity(true);
				}
#endif
			}
		} else {
			PIOS_Thread_Sleep(3);
		}
	}
}

/**
 * Transmit data buffer to the modem or USB port.
 * \param[in] data Data buffer to send
 * \param[in] length Length of buffer
 * \return -1 on failure
 * \return number of bytes transmitted on success
 */
static int32_t transmitData(uint8_t * data, int32_t length)
{
	uintptr_t outputPort = getComPort();

	if (outputPort)
		return PIOS_COM_SendBuffer(outputPort, data, length);

	return -1;
}

/**
 * Set update period of object (it must be already setup for periodic updates)
 * \param[in] obj The object to update
 * \param[in] updatePeriodMs The update period in ms, if zero then periodic updates are disabled
 * \return 0 Success
 * \return -1 Failure
 */
static int32_t setUpdatePeriod(UAVObjHandle obj, int32_t updatePeriodMs)
{
	UAVObjEvent ev;

	// Add object for periodic updates
	ev.obj = obj;
	ev.instId = UAVOBJ_ALL_INSTANCES;
	ev.event = EV_UPDATED_PERIODIC;
	return EventPeriodicQueueUpdate(&ev, queue, updatePeriodMs);
}

/**
 * Called each time the GCS telemetry stats object is updated.
 * Trigger a flight telemetry stats update if a connection is not
 * yet established.
 */
static void gcsTelemetryStatsUpdated()
{
	FlightTelemetryStatsData flightStats;
	GCSTelemetryStatsData gcsStats;
	FlightTelemetryStatsGet(&flightStats);
	GCSTelemetryStatsGet(&gcsStats);
	if (flightStats.Status != FLIGHTTELEMETRYSTATS_STATUS_CONNECTED || gcsStats.Status != GCSTELEMETRYSTATS_STATUS_CONNECTED) {
		updateTelemetryStats();
	}
}

/**
 * Update telemetry statistics and handle connection handshake
 */
static void updateTelemetryStats()
{
	UAVTalkStats utalkStats;
	FlightTelemetryStatsData flightStats;
	GCSTelemetryStatsData gcsStats;
	uint8_t forceUpdate;
	uint8_t connectionTimeout;
	uint32_t timeNow;

	// Get stats
	UAVTalkGetStats(uavTalkCon, &utalkStats);
	UAVTalkResetStats(uavTalkCon);

	// Get object data
	FlightTelemetryStatsGet(&flightStats);
	GCSTelemetryStatsGet(&gcsStats);

	// Update stats object
	if (flightStats.Status == FLIGHTTELEMETRYSTATS_STATUS_CONNECTED) {
		flightStats.RxDataRate = (float)utalkStats.rxBytes / ((float)STATS_UPDATE_PERIOD_MS / 1000.0f);
		flightStats.TxDataRate = (float)utalkStats.txBytes / ((float)STATS_UPDATE_PERIOD_MS / 1000.0f);
		flightStats.RxFailures += utalkStats.rxErrors;
		flightStats.TxFailures += txErrors;
		flightStats.TxRetries += txRetries;
		txErrors = 0;
		txRetries = 0;
	} else {
		flightStats.RxDataRate = 0;
		flightStats.TxDataRate = 0;
		flightStats.RxFailures = 0;
		flightStats.TxFailures = 0;
		flightStats.TxRetries = 0;
		txErrors = 0;
		txRetries = 0;
	}

	// Check for connection timeout
	timeNow = PIOS_Thread_Systime();
	if (utalkStats.rxObjects > 0) {
		timeOfLastObjectUpdate = timeNow;
	}
	if ((timeNow - timeOfLastObjectUpdate) > CONNECTION_TIMEOUT_MS) {
		connectionTimeout = 1;
	} else {
		connectionTimeout = 0;
	}

	// Update connection state
	forceUpdate = 1;
	if (flightStats.Status == FLIGHTTELEMETRYSTATS_STATUS_DISCONNECTED) {
		// Wait for connection request
		if (gcsStats.Status == GCSTELEMETRYSTATS_STATUS_HANDSHAKEREQ) {
			flightStats.Status = FLIGHTTELEMETRYSTATS_STATUS_HANDSHAKEACK;
		}
	} else if (flightStats.Status == FLIGHTTELEMETRYSTATS_STATUS_HANDSHAKEACK) {
		// Wait for connection
		if (gcsStats.Status == GCSTELEMETRYSTATS_STATUS_CONNECTED) {
			flightStats.Status = FLIGHTTELEMETRYSTATS_STATUS_CONNECTED;
		} else if (gcsStats.Status == GCSTELEMETRYSTATS_STATUS_DISCONNECTED) {
			flightStats.Status = FLIGHTTELEMETRYSTATS_STATUS_DISCONNECTED;
		}
	} else if (flightStats.Status == FLIGHTTELEMETRYSTATS_STATUS_CONNECTED) {
		if (gcsStats.Status != GCSTELEMETRYSTATS_STATUS_CONNECTED || connectionTimeout) {
			flightStats.Status = FLIGHTTELEMETRYSTATS_STATUS_DISCONNECTED;
		} else {
			forceUpdate = 0;
		}
	} else {
		flightStats.Status = FLIGHTTELEMETRYSTATS_STATUS_DISCONNECTED;
	}

	// Update the telemetry alarm
	if (flightStats.Status == FLIGHTTELEMETRYSTATS_STATUS_CONNECTED) {
		AlarmsClear(SYSTEMALARMS_ALARM_TELEMETRY);
	} else {
		AlarmsSet(SYSTEMALARMS_ALARM_TELEMETRY, SYSTEMALARMS_ALARM_ERROR);
	}

	// Update object
	FlightTelemetryStatsSet(&flightStats);

	// Force telemetry update if not connected
	if (forceUpdate) {
		FlightTelemetryStatsUpdated();
	}
}

/**
 * Update the telemetry settings, called on startup.
 */
static void updateSettings()
{
	if (PIOS_COM_TELEM_RF) {
		// Retrieve settings
		uint8_t speed;
		ModuleSettingsTelemetrySpeedGet(&speed);

		PIOS_HAL_ConfigureSerialSpeed(PIOS_COM_TELEM_RF, speed);
	}
}

/**
 * Determine input/output com port as highest priority available
 */
static uintptr_t getComPort()
{
#if defined(PIOS_INCLUDE_USB)
	if (PIOS_COM_Available(PIOS_COM_TELEM_USB)) {
		/* Let's further qualify this.  If there's anything spooled
		 * up for RX, bump the activity time.
		 */

		bool rx_pending = PIOS_COM_GetNumReceiveBytesPending(PIOS_COM_TELEM_USB) > 0;

		if (processUsbActivity(rx_pending)) {
			return PIOS_COM_TELEM_USB;
		}
	}
#endif /* PIOS_INCLUDE_USB */

	if (PIOS_COM_Available(PIOS_COM_TELEM_RF) )
		return PIOS_COM_TELEM_RF;

	return 0;
}

/**
 * SessionManaging object updated callback
 */
static void session_managing_updated(UAVObjEvent * ev, void *ctx, void *obj, int len)
{
	(void) ctx; (void) obj; (void) len;
	if (ev->event == EV_UNPACKED) {
		SessionManagingData sessionManaging;
		SessionManagingGet(&sessionManaging);

		if (sessionManaging.SessionID == 0) {
			sessionManaging.ObjectID = 0;
			sessionManaging.ObjectInstances = 0;
			sessionManaging.NumberOfObjects = UAVObjCount();
			sessionManaging.ObjectOfInterestIndex = 0;
		} else {
			uint8_t index = sessionManaging.ObjectOfInterestIndex;
			sessionManaging.ObjectID = UAVObjIDByIndex(index);
			sessionManaging.ObjectInstances = UAVObjGetNumInstances(UAVObjGetByID(sessionManaging.ObjectID));
		}
		SessionManagingSet(&sessionManaging);
	}
}

/**
 * New UAVO object instance callback
 * This is called from the uavobjectmanager
 * \param[in] obj_id The id of the object which had a new instance created
 * \param[in] inst_id the instance ID that was created
 */
static void update_object_instances(uint32_t obj_id, uint32_t inst_id)
{
	SessionManagingData sessionManaging;
	SessionManagingGet(&sessionManaging);
	sessionManaging.ObjectID = obj_id;
	sessionManaging.ObjectInstances = inst_id;
	sessionManaging.SessionID = sessionManaging.SessionID + 1;
	SessionManagingSet(&sessionManaging);
}

/**
  * @}
  * @}
  */
