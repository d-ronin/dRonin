/**
 ******************************************************************************
 * @addtogroup Modules Modules
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
#include "pios_thread.h"
#include "pios_mutex.h"
#include "pios_queue.h"

#include "pios_hal.h"

#include <uavtalk.h>

#ifndef TELEM_QUEUE_SIZE
/* 115200 = 11520 bytes/sec; if each transaction is 32 bytes,
 * this is 160ms of stuff.  Conversely, this is about 380 bytes
 * of memory if each obj event is 7 bytes-ish.
 */
#define TELEM_QUEUE_SIZE 60
#endif

#ifndef TELEM_STACK_SIZE
#define TELEM_STACK_SIZE 624
#endif

// Private constants
#define MAX_QUEUE_SIZE   TELEM_QUEUE_SIZE
#define STACK_SIZE_BYTES TELEM_STACK_SIZE
#define TASK_PRIORITY_RX PIOS_THREAD_PRIO_NORMAL
#define TASK_PRIORITY_TX PIOS_THREAD_PRIO_NORMAL
#define MAX_RETRIES 2

#define STATS_UPDATE_PERIOD_MS 1753
#define CONNECTION_TIMEOUT_MS 8000
#define USB_ACTIVITY_TIMEOUT_MS 6000

#define MAX_ACKS_PENDING 3
#define MAX_REQS_PENDING 5
#define ACK_TIMEOUT_MS 250

// Private types

// Private variables

struct pending_ack {
	UAVObjHandle obj;
	uint32_t timeout;

	uint16_t inst_id;
	uint8_t retry_count;
};

struct pending_req {
	uint32_t obj_id;
	uint16_t inst_id;
	char valid;
};

struct telemetry_state {
	struct pios_queue *queue;

	uint32_t tx_errors;
	uint32_t tx_retries;
	uint32_t time_of_last_update;

	struct pending_ack acks[MAX_ACKS_PENDING];
	struct pending_req reqs[MAX_REQS_PENDING];

	struct pios_mutex *reqack_mutex;

	UAVTalkConnection uavTalkCon;
};

static struct telemetry_state telem_state = { };

#if defined(PIOS_COM_TELEM_USB)
static volatile uint32_t usb_timeout_time;
#endif

typedef struct telemetry_state *telem_t;

// Private functions
static void telemetryTxTask(void *parameters);
static void telemetryRxTask(void *parameters);

static int32_t transmitData(void *ctx, uint8_t *data, int32_t length);
static void addAckPending(telem_t telem, UAVObjHandle obj, uint16_t inst_id);
static void ackCallback(void *ctx, uint32_t obj_id, uint16_t inst_id);
static void reqCallback(void *ctx, uint32_t obj_id, uint16_t inst_id);

static void registerObject(telem_t telem, UAVObjHandle obj);
static void updateObject(telem_t telem, UAVObjHandle obj, int32_t eventType);
static int32_t setUpdatePeriod(telem_t telem, UAVObjHandle obj, int32_t updatePeriodMs);
static void processObjEvent(telem_t telem, UAVObjEvent * ev);
static void updateTelemetryStats(telem_t telem);
static void gcsTelemetryStatsUpdated();
static void updateSettings();
static uintptr_t getComPort();
static void update_object_instances(uint32_t obj_id, uint32_t inst_id);

static int32_t fileReqCallback(void *ctx, uint8_t *buf,
                uint32_t file_id, uint32_t offset, uint32_t len);

static void registerObjectShim(UAVObjHandle obj) {
	registerObject(&telem_state, obj);
}

/**
 * Initialise the telemetry module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t TelemetryStart(void)
{
	// Process all registered objects and connect queue for updates
	UAVObjIterate(&registerObjectShim);

	// Create periodic event that will be used to update the telemetry stats
	UAVObjEvent ev;
	memset(&ev, 0, sizeof(UAVObjEvent));

	EventPeriodicQueueCreate(&ev, telem_state.queue, STATS_UPDATE_PERIOD_MS);

	// Listen to objects of interest
	GCSTelemetryStatsConnectQueue(telem_state.queue);

	struct pios_thread *telemetryTxTaskHandle;
	struct pios_thread *telemetryRxTaskHandle;

	// Start telemetry tasks
	telemetryTxTaskHandle = PIOS_Thread_Create(telemetryTxTask, "TelTx",
			STACK_SIZE_BYTES, &telem_state, TASK_PRIORITY_TX);
	telemetryRxTaskHandle = PIOS_Thread_Create(telemetryRxTask, "TelRx",
			STACK_SIZE_BYTES, &telem_state, TASK_PRIORITY_RX);

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
	if (FlightTelemetryStatsInitialize() == -1 ||
			GCSTelemetryStatsInitialize() == -1) {
		return -1;
	}

	telem_state.reqack_mutex = PIOS_Mutex_Create();

	if (!telem_state.reqack_mutex) {
		return -1;
	}

	// Initialize vars
	telem_state.time_of_last_update = 0;

	// Create object queues
	telem_state.queue = PIOS_Queue_Create(MAX_QUEUE_SIZE, sizeof(UAVObjEvent));

	// Initialise UAVTalk
	telem_state.uavTalkCon = UAVTalkInitialize(&telem_state, transmitData,
			ackCallback, reqCallback, fileReqCallback);

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
static void registerObject(telem_t telem, UAVObjHandle obj)
{
	if (UAVObjIsMetaobject(obj)) {
		/* Only connect change notifications for meta objects.  No periodic updates */
		UAVObjConnectQueue(obj, telem->queue, EV_MASK_ALL_UPDATES);
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
			EventPeriodicQueueCreate(&ev, telem->queue, 0);
		}

		// Setup object for telemetry updates
		updateObject(telem, obj, EV_NONE);
	}
}

/**
 * Update object's queue connections and timer, depending on object's settings
 * \param[in] obj Object to updates
 */
static void updateObject(telem_t telem, UAVObjHandle obj, int32_t eventType)
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
		setUpdatePeriod(telem, obj, metadata.telemetryUpdatePeriod);

		// Connect queue
		eventMask = EV_UPDATED_PERIODIC | EV_UPDATED_MANUAL;
		UAVObjConnectQueueThrottled(obj, telem->queue, eventMask, 0);
		break;
	case UPDATEMODE_ONCHANGE:
		// Set update period
		setUpdatePeriod(telem, obj, 0);

		// Connect queue
		eventMask = EV_UPDATED | EV_UPDATED_MANUAL;
		UAVObjConnectQueueThrottled(obj, telem->queue, eventMask, 0);
		break;
	case UPDATEMODE_THROTTLED:
		setUpdatePeriod(telem, obj, 0);

		eventMask = EV_UPDATED | EV_UPDATED_MANUAL;
		UAVObjConnectQueueThrottled(obj, telem->queue, eventMask,
				metadata.telemetryUpdatePeriod);
		break;
	case UPDATEMODE_MANUAL:
		// Set update period
		setUpdatePeriod(telem, obj, 0);

		// Connect queue
		eventMask = EV_UPDATED_MANUAL;
		UAVObjConnectQueueThrottled(obj, telem->queue, eventMask, 0);
		break;
	}
}

/**
 * Either expires or retransmits a message that expects ack.
 *
 * \param[in] telem Telemetry subsystem handle
 * \param[in] idx The offset for "which ack" to do this for.
 */
static void ackResendOrTimeout(telem_t telem, int idx)
{
	if (telem->acks[idx].retry_count++ > MAX_RETRIES) {
		DEBUG_PRINTF(3, "telem: abandoning ack wait for %d/%d\n",
				UAVObjGetID(telem->acks[idx].obj),
				telem->acks[idx].inst_id);

		/* Two different kinds of things in tx_errors-- objects that
		 * failed to transfer, and send errors (shouldn't happen) */
		telem->tx_errors++;

		telem->acks[idx].obj = NULL;
	} else {
		DEBUG_PRINTF(3, "telem: abandoning ack wait for %d/%d\n",
				UAVObjGetID(telem->acks[idx].obj),
				telem->acks[idx].inst_id);

		int32_t success;

		telem->acks[idx].timeout = PIOS_Thread_Systime() +
			ACK_TIMEOUT_MS;

		/* Must not hold lock while sending an object, because
		 * of lock ordering issues (though the lock should be
		 * severely squashed in uavtalk.c
		 */
		PIOS_Mutex_Unlock(telem->reqack_mutex);
		success = UAVTalkSendObject(telem->uavTalkCon,
				telem->acks[idx].obj,
				telem->acks[idx].inst_id,
				true);
		PIOS_Mutex_Lock(telem->reqack_mutex, PIOS_MUTEX_TIMEOUT_MAX);

		telem->tx_retries++;

		if (success == -1) {
			telem->tx_errors++;
		}
	}
}

/**
 * Checks for expected-ack messages that need resend or expiration.
 *
 * \param[in] telem Telemetry subsystem handle
 */
static bool ackHousekeeping(telem_t telem)
{
	bool did_something = false;

	uint32_t tm = PIOS_Thread_Systime();

	for (int i = 0; i < MAX_ACKS_PENDING; i++) {
		if (telem->acks[i].obj) {
			if (tm > telem->acks[i].timeout) {
				ackResendOrTimeout(telem, i);

				did_something = true;
			}
		}
	}

	return did_something;
}

/**
 * Adds a message for which we want an ack.  Can block if there are already
 * too many ack-required things pending.
 *
 * \param[in] telem Telemetry subsystem handle
 * \param[in] obj The object for which we want to wait for an ack.
 * \param[in] inst_id The instance ID of said object.
 */
static void addAckPending(telem_t telem, UAVObjHandle obj, uint16_t inst_id)
{
	DEBUG_PRINTF(3, "telem: want ack for %d/%d\n", UAVObjGetID(obj),
			inst_id);

	while (true) {
		PIOS_Mutex_Lock(telem->reqack_mutex, PIOS_MUTEX_TIMEOUT_MAX);
		ackHousekeeping(telem);

		/* There's a slight race here, if we're --already-- waiting
		 * for an ack on an object, we can end up putting it in here
		 * to wait again.  In a perfect storm-- past retries cause two
		 * acks, and then the new object is lost-- we can be fooled.
		 * It seems like a lesser evil than most of the alternatives
		 * that keep good pipelining, though
		 */
		for (int i = 0; i < MAX_ACKS_PENDING; i++) {
			if (!telem->acks[i].obj) {
				telem->acks[i].obj = obj;
				telem->acks[i].inst_id = inst_id;
				telem->acks[i].timeout =
					PIOS_Thread_Systime() +
					ACK_TIMEOUT_MS;

				telem->acks[i].retry_count = 0;

				PIOS_Mutex_Unlock(telem->reqack_mutex);
				return;
			}
		}

		DEBUG_PRINTF(3, "telem: blocking because acks are full\n");

		/* Oh no.  This is the bad case--- maximum number of things
		 * needing ack in flight.
		 */

		PIOS_Mutex_Unlock(telem->reqack_mutex);

		/* TODO: Consider nicer wakeup mechanism here. */
		PIOS_Thread_Sleep(3);
	}
}

/**
 * Callback for when we receive a request for data.  Converts a file
 * id to the actual unit of information, and returns/copies it.
 * Currently only operates on partitions.
 *
 * \param[in] ctx Callback context (telemetry subsystem handle)
 * \param[in] file_id The requested file_id
 * \param[in] offset The offset of information requested
 * \param[in] len The maximum amount of information to return.  A short read
 * is permissable, as long as some bytes are returned.
 * \returns The number of bytes filled, 0 on EOF, negative on error.
 */
static int32_t fileReqCallback(void *ctx, uint8_t *buf,
                uint32_t file_id, uint32_t offset, uint32_t len)
{
	if (file_id < FLASH_PARTITION_NUM_LABELS) {
		uintptr_t part_id;

		if (PIOS_FLASH_find_partition_id(file_id, &part_id)) {
			return -1;
		}

		uint32_t size;

		if (PIOS_FLASH_get_partition_size(part_id, &size)) {
			return -2;
		}

		if (offset >= size) {
			return 0;
		}

		uint32_t remaining = size - offset;

		if (len > remaining) {
			len = remaining;
		}

		if (PIOS_FLASH_start_transaction(part_id)) {
			return -3;
		}

		if (PIOS_FLASH_read_data(part_id, offset, buf, len)) {
			len = -4;
		}

		PIOS_FLASH_end_transaction(part_id);

		return len;
	}

	return -1;
}

/**
 * Callback for when a telemetry session requests data
 *
 * \param[in] ctx Callback context (telemetry subsystem handle)
 * \param[in] obj_id The object ID that we got a req for.
 * \param[in] inst_id The instance ID that we got a req for.
 */
static void reqCallback(void *ctx, uint32_t obj_id, uint16_t inst_id)
{
	telem_t telem = ctx;

	PIOS_Mutex_Lock(telem->reqack_mutex,
			PIOS_MUTEX_TIMEOUT_MAX);

	int i;

	for (i = 0; i < MAX_REQS_PENDING - 1; i++) {
		if (!telem->reqs[i].valid) {
			break;
		}
	}

	if (i < MAX_REQS_PENDING) {
		telem->reqs[i].valid = 1;
		telem->reqs[i].obj_id = obj_id;
		telem->reqs[i].inst_id = inst_id;
	}

	// Unlock, to ensure new requests can come in OK.
	PIOS_Mutex_Unlock(telem->reqack_mutex);
}

/**
 * Callback for when we receive an ack.
 *
 * \param[in] ctx Callback context (telemetry subsystem handle)
 * \param[in] obj_id The object ID that we got an ack for.
 * \param[in] inst_id The instance ID that we got an ack for.
 */
static void ackCallback(void *ctx, uint32_t obj_id, uint16_t inst_id)
{
	telem_t telem = ctx;

	PIOS_Mutex_Lock(telem->reqack_mutex, PIOS_MUTEX_TIMEOUT_MAX);

	for (int i = 0; i < MAX_ACKS_PENDING; i++) {
		if (!telem->acks[i].obj) {
			continue;
		}

		if (inst_id != telem->acks[i].inst_id) {
			continue;
		}

		uint32_t other_obj_id = UAVObjGetID(telem->acks[i].obj);

		if (obj_id != other_obj_id) {
			continue;
		}

		telem->acks[i].obj = NULL;

		DEBUG_PRINTF(3, "telem: Got ack for %d/%d\n", obj_id, inst_id);
		PIOS_Mutex_Unlock(telem->reqack_mutex);
		return;
	}

	PIOS_Mutex_Unlock(telem->reqack_mutex);

	DEBUG_PRINTF(3, "telem: Got UNEXPECTED ack for %d/%d\n", obj_id, inst_id);
}

/**
 * Processes queue events
 */
static void processObjEvent(telem_t telem, UAVObjEvent * ev)
{
	if (ev->obj == 0) {
		updateTelemetryStats(telem);
	} else if (ev->obj == GCSTelemetryStatsHandle()) {
		gcsTelemetryStatsUpdated(telem);
	} else {
		// Act on event
		if (ev->event == EV_UPDATED || ev->event == EV_UPDATED_MANUAL ||
				ev->event == EV_UPDATED_PERIODIC) {
			int32_t success = -1;

			UAVObjMetadata metadata;

			bool acked = false;

			// Get object metadata
			UAVObjGetMetadata(ev->obj, &metadata);

			if (UAVObjGetTelemetryAcked(&metadata)) {
				acked = true;

				addAckPending(telem, ev->obj, ev->instId);
			}

			success = UAVTalkSendObject(telem->uavTalkCon,
					ev->obj, ev->instId,
					acked);

			if (success == -1) {
				telem->tx_errors++;
			}
		}

		// If this is a metaobject then make necessary
		// telemetry updates
		if (UAVObjIsMetaobject(ev->obj)) {
			// linked object will be the actual object the
			// metadata are for
			updateObject(telem, UAVObjGetLinkedObj(ev->obj),
					EV_NONE);
		}
	}
}

static bool sendRequestedObjs(telem_t telem)
{
	// Must be called with the reqack mutex.
	if (!telem->reqs[0].valid) {
		return false;
	}

	struct pending_req preq = telem->reqs[0];

	/* Expensive, but for as little as this comes up,
	 * who cares.
	 */
	int i;

	for (i = 1; i < MAX_REQS_PENDING; i++) {
		if (!telem->reqs[i].valid) {
			break;
		}
		telem->reqs[i-1] = telem->reqs[i];
	}

	telem->reqs[i-1].valid = 0;

	// Unlock, to ensure new requests can come in OK.
	PIOS_Mutex_Unlock(telem->reqack_mutex);

	// handle request
	UAVObjHandle obj = UAVObjGetByID(preq.obj_id);

	// Send requested object if message is of type OBJ_REQ
	if (!obj) {
		UAVTalkSendNack(telem->uavTalkCon, preq.obj_id,
				preq.inst_id);
	} else {
		if ((preq.inst_id == UAVOBJ_ALL_INSTANCES) ||
		    (preq.inst_id < UAVObjGetNumInstances(obj))) {
			UAVTalkSendObject(telem->uavTalkCon,
					obj, preq.inst_id,
					false);
		} else {
			UAVTalkSendNack(telem->uavTalkCon,
					preq.obj_id,
					preq.inst_id);
		}
	}


	PIOS_Mutex_Lock(telem->reqack_mutex,
			PIOS_MUTEX_TIMEOUT_MAX);

	return true;
}

/**
 * Telemetry transmit task
 */
static void telemetryTxTask(void *parameters)
{
	telem_t telem = parameters;

	// Update telemetry settings
	updateSettings();

	// Loop forever
	while (1) {
		UAVObjEvent ev;

		bool retval;

		// Wait for queue message or short timeout
		retval = PIOS_Queue_Receive(telem->queue, &ev, 10);

		PIOS_Mutex_Lock(telem->reqack_mutex,
				PIOS_MUTEX_TIMEOUT_MAX);

		/* Short-circuit means sendRequestedObjs "wins" */
		while (sendRequestedObjs(telem) || ackHousekeeping(telem));

		PIOS_Mutex_Unlock(telem->reqack_mutex);

		if (retval == true) {
			// Process event
			processObjEvent(telem, &ev);
		}

	}
}

#if defined(PIOS_COM_TELEM_USB)
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
#endif // PIOS_COM_TELEM_USB

/**
 * Telemetry transmit task. Processes queue events and periodic updates.
 */
static void telemetryRxTask(void *parameters)
{
	telem_t telem = parameters;

	// Task loop
	while (1) {
		uintptr_t inputPort = getComPort();

		if (inputPort) {
			// Block until data are available
			uint8_t serial_data[16];
			uint16_t bytes_to_process;

			bytes_to_process = PIOS_COM_ReceiveBuffer(inputPort, serial_data, sizeof(serial_data), 500);

			if (bytes_to_process > 0) {
				UAVTalkProcessInputStream(telem->uavTalkCon, serial_data, bytes_to_process);

#if defined(PIOS_COM_TELEM_USB)
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
static int32_t transmitData(void *ctx, uint8_t * data, int32_t length)
{
	(void) ctx;

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
static int32_t setUpdatePeriod(telem_t telem, UAVObjHandle obj,
		int32_t updatePeriodMs)
{
	UAVObjEvent ev;

	// Add object for periodic updates
	ev.obj = obj;
	ev.instId = UAVOBJ_ALL_INSTANCES;
	ev.event = EV_UPDATED_PERIODIC;
	return EventPeriodicQueueUpdate(&ev, telem->queue, updatePeriodMs);
}

/**
 * Called each time the GCS telemetry stats object is updated.
 * Trigger a flight telemetry stats update if a connection is not
 * yet established.
 */
static void gcsTelemetryStatsUpdated(telem_t telem)
{
	FlightTelemetryStatsData flightStats;
	GCSTelemetryStatsData gcsStats;
	FlightTelemetryStatsGet(&flightStats);
	GCSTelemetryStatsGet(&gcsStats);
	if (flightStats.Status != FLIGHTTELEMETRYSTATS_STATUS_CONNECTED || gcsStats.Status != GCSTELEMETRYSTATS_STATUS_CONNECTED) {
		updateTelemetryStats(telem);
	}
}

/**
 * Update telemetry statistics and handle connection handshake
 */
static void updateTelemetryStats(telem_t telem)
{
	UAVTalkStats utalkStats;
	FlightTelemetryStatsData flightStats;
	GCSTelemetryStatsData gcsStats;
	uint8_t forceUpdate;
	uint8_t connectionTimeout;
	uint32_t timeNow;

	// Get stats
	UAVTalkGetStats(telem->uavTalkCon, &utalkStats);

	// Get object data
	FlightTelemetryStatsGet(&flightStats);
	GCSTelemetryStatsGet(&gcsStats);

	// Update stats object
	if (flightStats.Status == FLIGHTTELEMETRYSTATS_STATUS_CONNECTED) {
		/* This is bogus because updates to GCSTelemetryStats trigger
		 * this call, in addition to just the periodic stuff.
		 */
		flightStats.RxDataRate = (float)utalkStats.rxBytes / ((float)STATS_UPDATE_PERIOD_MS / 1000.0f);
		flightStats.TxDataRate = (float)utalkStats.txBytes / ((float)STATS_UPDATE_PERIOD_MS / 1000.0f);
		flightStats.RxFailures += utalkStats.rxErrors;
		flightStats.TxFailures += telem->tx_errors;
		flightStats.TxRetries += telem->tx_retries;
		telem->tx_errors = 0;
		telem->tx_retries = 0;
	} else {
		flightStats.RxDataRate = 0;
		flightStats.TxDataRate = 0;
		flightStats.RxFailures = 0;
		flightStats.TxFailures = 0;
		flightStats.TxRetries = 0;
		telem->tx_errors = 0;
		telem->tx_retries = 0;
	}

	// Check for connection timeout
	timeNow = PIOS_Thread_Systime();
	if (utalkStats.rxObjects > 0) {
		telem->time_of_last_update = timeNow;
	}
	if ((timeNow - telem->time_of_last_update) > CONNECTION_TIMEOUT_MS) {
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
	if (PIOS_COM_TELEM_SER) {
		// Retrieve settings
		uint8_t speed;
		ModuleSettingsTelemetrySpeedGet(&speed);

		PIOS_HAL_ConfigureSerialSpeed(PIOS_COM_TELEM_SER, speed);
	}
}

/**
 * Determine input/output com port as highest priority available
 */
static uintptr_t getComPort()
{
#if defined(PIOS_COM_TELEM_USB)
	if (PIOS_COM_Available(PIOS_COM_TELEM_USB)) {
		/* Let's further qualify this.  If there's anything spooled
		 * up for RX, bump the activity time.
		 */

		bool rx_pending = PIOS_COM_GetNumReceiveBytesPending(PIOS_COM_TELEM_USB) > 0;

		if (processUsbActivity(rx_pending)) {
			return PIOS_COM_TELEM_USB;
		}
	}
#endif /* PIOS_COM_TELEM_USB */

	if (PIOS_COM_Available(PIOS_COM_TELEM_SER))
		return PIOS_COM_TELEM_SER;

	return 0;
}

/**
 * New UAVO object instance callback
 * This is called from the uavobjectmanager
 * \param[in] obj_id The id of the object which had a new instance created
 * \param[in] inst_id the instance ID that was created
 */
static void update_object_instances(uint32_t obj_id, uint32_t inst_id)
{
	/* XXX just send the new instance, reqack. */
}

/**
  * @}
  * @}
  */
