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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "openpilot.h"
#include "pios_hal.h"
#include "msp.h"

#include "modulesettings.h"
#include "attitudeactual.h"
#include "accels.h"
#include "gyros.h"
#include "flightbatterystate.h"

#if defined(PIOS_MSP_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_MSP_STACK_SIZE
#else
#define STACK_SIZE_BYTES 800
#endif
#define THREAD_PRIORITY PIOS_THREAD_PRIO_LOW

/* time to wait for a reply */
#define MSP_TIMEOUT 10 // ms


struct msp_bridge {
	struct pios_com_dev *com;
	struct msp_parser *parser;

	volatile enum msp_message_id expected;
	volatile bool done;
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

static bool msp_unpack_attitude(void *data, uint8_t len)
{
	struct msp_attitude *att = data;
	if (len < sizeof(*att))
		return false;

	AttitudeActualData attActual;
	AttitudeActualGet(&attActual);

	// Roll and Pitch are in 10ths of a degree.
	attActual.Roll = (float)att->angx / 10.0f;
	attActual.Pitch = (float)att->angy / -10.0f;
	// Yaw is just -180 -> 180
	attActual.Yaw = att->heading;

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

	return true;
}

static void set_baudrate(struct msp_bridge *m)
{
	if (!m->com)
		return;

	uint8_t baud;
	ModuleSettingsMSPSpeedGet(&baud);
	/* TODO: teach pios_com to take a real pointer */
	PIOS_HAL_ConfigureSerialSpeed((uintptr_t)m->com, baud);
}

static bool msp_handler(enum msp_message_id msg_id, void *data, uint8_t len, void *context)
{
	if (!context)
		return false;
	struct msp_bridge *msp = context;

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

	if (msg_id == msp->expected)
		msp->done = true;

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
 * @retval -2 failed to create task
 */
static int32_t mspUavoBridge_Start(void)
{
	if (!module_enabled)
		return -1;

	struct pios_thread *task = PIOS_Thread_Create(mspUavoBridge_Task,
		"MspUavoBridge", STACK_SIZE_BYTES, NULL, THREAD_PRIORITY);

	if (!task)
		return -2;

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
	AccelsInitialize();
	GyrosInitialize();
	FlightBatteryStateInitialize();

	if (!(AccelsHandle() && GyrosHandle() && FlightBatteryStateHandle()))
		return -3;

	return 0;
}

MODULE_INITCALL(mspUavoBridge_Init, mspUavoBridge_Start)

/**
 * @}
 * @}
 */
