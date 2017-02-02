/**
 ******************************************************************************
 * @file       mspuavobridge.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup Modules dRonin Modules
 * @{
 * @addtogroup MspUavoBridge MSP to UAVO Bridge
 * @{
 * @brief Bridges MSP (Multiwii Serial Protocol) telemetry to UAVOs
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
#include "pios_hal.h"
#include "coordinate_conversions.h"
#include "msp.h"

#include "modulesettings.h"
#include "attitudeactual.h"
#include "accels.h"
#include "gyros.h"
#include "flightbatterystate.h"
#include "manualcontrolcommand.h"

#if defined(PIOS_MSP_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_MSP_STACK_SIZE
#else
#define STACK_SIZE_BYTES 800
#endif
#define THREAD_PRIORITY PIOS_THREAD_PRIO_LOW

/* time to wait for a reply */
#define MSP_TIMEOUT 10 // ms
#define MSP_BRIDGE_MAGIC 0x97a4dc58 


struct msp_bridge {
	uint32_t magic;

	struct pios_com_dev *com;
	struct msp_parser *parser;

	enum msp_message_id expected;
	bool done;
};

static struct msp_message_list_item {
	uint32_t last_fetched;
	uint16_t period; /* ms */
	enum msp_message_id msg_id;
} message_list[] = {
	/* these are the messages that will be fetched and their intervals */
	{ .msg_id = MSP_MSG_ATTITUDE, .period = 20 },
	{ .msg_id = MSP_MSG_ANALOG,   .period = 100 },
	{ .msg_id = MSP_MSG_RAW_IMU,  .period = 50 },
};

static bool module_enabled = false;
static struct msp_bridge *msp;

// TODO: ick, use a real pointer
extern uintptr_t pios_com_msp_id;


static bool msp_bridge_validate(struct msp_bridge *m)
{
	return m != NULL && m->magic == MSP_BRIDGE_MAGIC;
}

static bool msp_unpack_attitude(void *data, uint8_t len)
{
	struct msp_attitude *att = data;
	if (len < sizeof(*att))
		return false;

	AttitudeActualData attActual;
	AttitudeActualGet(&attActual);

	const float rpy[3] = {
		(float)att->angx / 10.0f, /* [-1800:1800] 1/10 deg */
		(float)att->angy / -10.0f, /* [-900:900] 1/10 deg */
		att->heading /* [-180:180] deg */
	};
	attActual.Roll = rpy[0];
	attActual.Pitch = rpy[1];
	attActual.Yaw = rpy[2];

	float q[4];
	RPY2Quaternion(rpy, q);
	attActual.q1 = q[0];
	attActual.q2 = q[1];
	attActual.q3 = q[2];
	attActual.q4 = q[3];

	AttitudeActualSet(&attActual);

	return true;
}

static bool msp_unpack_raw_imu(void *data, uint8_t len)
{
	struct msp_raw_imu *imu = data;
	if (len < sizeof(*imu))
		return false;

	AccelsData accels;
	AccelsGet(&accels);
	/* based on MPU 6050 numbers given in protocol spec */
	accels.x = (float)imu->accx * 9.8f / 512;
	accels.y = (float)imu->accy * 9.8f / 512;
	accels.z = (float)imu->accz * 9.8f / 512;
	AccelsSet(&accels);

	GyrosData gyros;
	GyrosGet(&gyros);
	/* based on MPU 6050 numbers given in protocol spec */
	gyros.x = (float)imu->gyrx / 4.096f;
	gyros.y = (float)imu->gyry / 4.096f;
	gyros.z = (float)imu->gyrz / 4.096f;
	GyrosSet(&gyros);

	return true;
}

static bool msp_unpack_analog(void *data, uint8_t len)
{
	struct msp_analog *analog = data;
	if (len < sizeof(*analog))
		return false;

	FlightBatteryStateData batt;
	FlightBatteryStateGet(&batt);
	batt.Voltage = (float)analog->vbat / 10.0f;
	/* TODO: check scaling */
	batt.Current = (float)analog->amperage;
	batt.ConsumedEnergy = (float)analog->power_meter_sum;
	FlightBatteryStateSet(&batt);

	ManualControlCommandData cntrl;
	ManualControlCommandGet(&cntrl);
	cntrl.RawRssi = analog->rssi;
	cntrl.Rssi = analog->rssi * 100 / 1024; /* [0:1023] */
	ManualControlCommandSet(&cntrl);

	return true;
}

static void set_baudrate(struct msp_bridge *m)
{
	PIOS_Assert(msp_bridge_validate(m));
	if (!m->com)
		return;

	uint8_t baud;
	ModuleSettingsMSPSpeedGet(&baud);
	/* TODO: teach pios_com to take a real pointer */
	PIOS_HAL_ConfigureSerialSpeed((uintptr_t)m->com, baud);
}

static bool msp_handler(enum msp_message_id msg_id, void *data, uint8_t len, void *context)
{
	struct msp_bridge *m = context;
	if (!msp_bridge_validate(m))
		return false;

	bool retval = false;
	switch (msg_id) {
	case MSP_MSG_ATTITUDE:
		retval &= msp_unpack_attitude(data, len);
		break;
	case MSP_MSG_RAW_IMU:
		retval &= msp_unpack_raw_imu(data, len);
		break;
	case MSP_MSG_ANALOG:
		retval &= msp_unpack_analog(data, len);
		break;
	default:
		break;
	}

	if (msg_id == m->expected)
		m->done = true;

	return retval;
}

static enum msp_message_id get_next_message(void)
{
	for (int i = 0; i < NELEMENTS(message_list); i++) {
		struct msp_message_list_item *item = &message_list[i];
		if (PIOS_Thread_Period_Elapsed(item->last_fetched, item->period)) {
			item->last_fetched = PIOS_Thread_Systime();
			return item->msg_id;
		}
	}
	return 0;
}

/**
 * @brief MSP to UAVO bridge main task
 * @param[in] parameters Unused
 */
static void mspUavoBridge_Task(void *parameters)
{
	set_baudrate(msp);
	msp_register_handler(msp->parser, msp_handler, msp);

	while(true) {
		PIOS_Assert(msp_bridge_validate(msp));
		msp->done = false;

		while (!(msp->expected = get_next_message()))
			PIOS_Thread_Sleep(1);

		msp_send_com(msp->parser, msp->com, msp->expected, NULL, 0);

		int time = 0;
		while (!msp->done && time < MSP_TIMEOUT) {
			msp_process_com(msp->parser, msp->com);
			if (!msp->done) {
				time++;
				PIOS_Thread_Sleep(1);
			}
		}
	}
}

/**
 * @brief Start the MSP to UAVO bridge module
 * @retval 0 success
 * @retval -1 module not enabled
 * @retval -2 invalid msp bridge state, somebody else blew stack?
 * @retval -3 failed to create task
 */
static int32_t mspUavoBridge_Start(void)
{
	if (!module_enabled)
		return -1;

	if (!msp_bridge_validate(msp))
		return -2;

	struct pios_thread *task = PIOS_Thread_Create(mspUavoBridge_Task,
		"MspUavoBridge", STACK_SIZE_BYTES, NULL, THREAD_PRIORITY);

	if (!task)
		return -3;

	TaskMonitorAdd(TASKINFO_RUNNING_MSPUAVOBRIDGE, task);

	return 0;
}

/**
 * @brief Initialize the MSP to UAVO bridge module
 * @retval 0 success
 * @retval -1 failed to allocate memory
 * @retval -2 failed to initialize MSP parser
 * @retval -3 one or more UAVOs not available
 */
static int32_t mspUavoBridge_Init(void)
{
	// TODO: PIOS_MODULE_MSPUAVOBRIDGE
	if (pios_com_msp_id && PIOS_Modules_IsEnabled(PIOS_MODULE_UAVOMSPBRIDGE))
		module_enabled = true;

	msp = PIOS_malloc(sizeof(*msp));
	if (!msp) {
		module_enabled = false;
		return -1;
	}

	memset(msp, 0, sizeof(*msp));

	msp->parser = msp_parser_init(MSP_PARSER_CLIENT);
	if (!msp->parser) {
		PIOS_free(msp);
		return -2;
	}

	/* TODO: make pios_com use real pointer */
	msp->com = (struct pios_com_dev *)pios_com_msp_id;

	/* Init these in case they aren't already (we don't want to die) */
	bool check = true;
	/* WARNING: this macro has side-effects! */
	#define INIT_CHECK_UAVO(name) name##Initialize(); check &= (name##Handle() != NULL)
	INIT_CHECK_UAVO(Accels);
	INIT_CHECK_UAVO(AttitudeActual);
	INIT_CHECK_UAVO(Gyros);
	INIT_CHECK_UAVO(FlightBatteryState);
	INIT_CHECK_UAVO(ManualControlCommand);
	if (!check)
		return -3;

	msp->magic = MSP_BRIDGE_MAGIC;

	return 0;
}

MODULE_INITCALL(mspUavoBridge_Init, mspUavoBridge_Start)

/**
 * @}
 * @}
 */
