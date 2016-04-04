/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup Storm32bgc Storm32bgc Module
 * @{
 *
 * @file       storm32bgc.c
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#include "pios_modules.h"

#include "cameradesired.h"

// Private constants
#define STACK_SIZE_BYTES 512
#define TASK_PRIORITY PIOS_THREAD_PRIO_LOW

#define STORM32BGC_PERIOD_MS  20  // 50 Hz
#define STORM32BGC_QUEUE_SIZE 64

// Private types

// Private variables
static struct pios_thread          * storm32bgcTaskHandle;
static bool                          module_enabled;
struct pios_queue                  * storm32bgc_queue;
static struct pios_recursive_mutex * mutex;

// Private functions
static void storm32bgcTask(void *parameters);

static inline uint16_t crc_calculate(uint8_t* pBuffer, int length);

static uint8_t cmd_set_angle(float pitch, float roll, float yaw, uint8_t flags, uint8_t type);

// Local variables
static uintptr_t storm32bgc_com_id;

/**
 * Initialise the Storm32Bgc module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t Storm32BgcInitialize(void)
{
	storm32bgc_com_id = PIOS_COM_STORM32BGC;

	module_enabled = PIOS_Modules_IsEnabled(PIOS_MODULE_STORM32BGC);

	if (!storm32bgc_com_id)
		module_enabled = false;

	if (!module_enabled)
		return -1;

	return 0;
}

/**
 * Start the Storm32Bgc module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t Storm32BgcStart(void)
{
	//Check if module is enabled or not
	if (module_enabled == false) {
		return -1;
	}

	// create storm32bgc queue
	storm32bgc_queue = PIOS_Queue_Create(STORM32BGC_QUEUE_SIZE, sizeof(UAVObjEvent));
	if (!storm32bgc_queue){
		return -1;
	}

	// Create mutex
	mutex = PIOS_Recursive_Mutex_Create();
	if (mutex == NULL){
		return -2;
	}

	// Start storm32bgc task
	storm32bgcTaskHandle = PIOS_Thread_Create(storm32bgcTask, "Storm32bgc", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);

	TaskMonitorAdd(TASKINFO_RUNNING_STORM32BGC, storm32bgcTaskHandle);

	return 0;
}

MODULE_INITCALL(Storm32BgcInitialize, Storm32BgcStart);

static void storm32bgcTask(void *parameters)
{
	uint32_t now = PIOS_Thread_Systime();

	CameraDesiredData cameraDesired;

	// Loop forever
	while (1)
	{
		PIOS_Thread_Sleep_Until(&now, STORM32BGC_PERIOD_MS);

		CameraDesiredGet(&cameraDesired);

		float pitch_setpoint = cameraDesired.Declination;
		float yaw_setpoint   = cameraDesired.Bearing;

		switch(cmd_set_angle(pitch_setpoint, 0.0f, yaw_setpoint, 0x00, 0x00)) {

			case 0:  // No response from Storm32Bgc
				AlarmsSet(SYSTEMALARMS_ALARM_GIMBAL, SYSTEMALARMS_ALARM_WARNING);
				break;

			case 1:  // ACK response from Storm32Bgc
				AlarmsSet(SYSTEMALARMS_ALARM_GIMBAL, SYSTEMALARMS_ALARM_OK);
				break;

			default: // ACK response from Storm32Bgc invalid
				AlarmsSet(SYSTEMALARMS_ALARM_GIMBAL, SYSTEMALARMS_ALARM_CRITICAL);
		}
	}
}

union {
	uint16_t value;
	uint8_t bytes[2];
} crc;

union {
	uint16_t value;
	uint8_t bytes[2];
} dataU16;

union {
	float value;
	uint8_t bytes[4];
} dataFloat;

/**
 *
 *  CALCULATE THE CHECKSUM
 *
 */

#define X25_INIT_CRC 0xFFFF

/**
 * @brief Accumulate the X.25 CRC by adding one char at a time.
 *
 * The checksum function adds the hash of one char at a time to the
 * 16 bit checksum (uint16_t).
 *
 * @param data new char to hash
 * @param crcAccum the already accumulated checksum
 **/
static inline void crc_accumulate(uint8_t data, uint16_t *crcAccum)
{
	/*Accumulate one byte of data into the CRC*/
	uint8_t tmp;

	tmp = data ^ (uint8_t)(*crcAccum &0xFF);
	tmp ^= (tmp<<4);
	*crcAccum = (*crcAccum>>8) ^ (tmp<<8) ^ (tmp <<3) ^ (tmp>>4);
}

/**
 * @brief Calculates the X.25 checksum on a byte buffer
 *
 * @param  pBuffer buffer containing the byte array to hash
 * @param  length  length of the byte array
 * @return the checksum over the buffer bytes
 **/
static inline uint16_t crc_calculate(uint8_t* pBuffer, int length)
{
	// For a "message" of length bytes contained in the unsigned char array
	// pointed to by pBuffer, calculate the CRC
	// crcCalculate(unsigned char* pBuffer, int length, unsigned short* checkConst) < not needed

	uint16_t crcTmp;
	uint8_t* pTmp;
	int i;

	pTmp=pBuffer;

	/* init crcTmp */
	crcTmp = X25_INIT_CRC;

	for (i = 0; i < length; i++){
		crc_accumulate(*pTmp++, &crcTmp);
	}

	return(crcTmp);
}

/**
 * Send the Set Angle Command
 * \return   0 = No Response from Storm32bgc
 * \return   1 = SERIALRCCMD_ACK_OK
 * \return   2 = SERIALRCCMD_ACK_ERR_FAIL
 * \return   3 = SERIALRCCMD_ACK_ERR_ACCESS_DENIED
 * \return   4 = SERIALRCCMD_ACK_ERR_NOT_SUPPORTED
 * \return 151 = SERIALRCCMD_ACK_ERR_TIMEOUT
 * \return 152 = SERIALRCCMD_ACK_ERR_CRC
 * \return 153 = SERIALRCCMD_ACK_ERR_PAYLOADLEN
 * \return 200 = Storm32bgc ACK CRC incorrect
 */
static uint8_t cmd_set_angle(float pitch, float roll, float yaw, uint8_t flags, uint8_t type)
{
	uint8_t command_string[19];
	uint8_t read_data[6] = {0,};
	uint8_t bytes_read;
	uint8_t ack;

	command_string[0] = 0xFA;  // Start Sign
	command_string[1] = 0x0E;  // Length of Payload
	command_string[2] = 0x11;  // Command Byte - CMD_SETANGLE

	dataFloat.value = pitch;
	command_string[3] = dataFloat.bytes[0];  // pitch-byte0
	command_string[4] = dataFloat.bytes[1];  // pitch-byte1
	command_string[5] = dataFloat.bytes[2];  // pitch-byte2
	command_string[6] = dataFloat.bytes[3];  // pitch-byte3

	dataFloat.value = roll;
	command_string[7]  = dataFloat.bytes[0];  // roll-byte0
	command_string[8]  = dataFloat.bytes[1];  // roll-byte1
	command_string[9]  = dataFloat.bytes[2];  // roll-byte2
	command_string[10] = dataFloat.bytes[3];  // roll-byte3

	dataFloat.value = yaw;
	command_string[11] = dataFloat.bytes[0];  // yaw-byte0
	command_string[12] = dataFloat.bytes[1];  // yaw-byte1
	command_string[13] = dataFloat.bytes[2];  // yaw-byte2
	command_string[14] = dataFloat.bytes[3];  // yaw-byte3

	command_string[15] = flags;  // flags-byte

	command_string[16] = type;   // type-byte

	crc.value = crc_calculate(&command_string[1], 16);  // Does not include Start Sign

	command_string[17] = crc.bytes[0];
	command_string[18] = crc.bytes[1];

	PIOS_COM_SendBuffer(storm32bgc_com_id, &command_string[0], 19);

	bytes_read = PIOS_COM_ReceiveBuffer(storm32bgc_com_id, read_data, 6, 500);

	if (bytes_read != 0)
	{
		crc.value = crc_calculate(&read_data[1], 3);  // Does not include Start Sign

		if ((crc.bytes[0] == read_data[4]) && (crc.bytes[1] == read_data[5]))
		{
			ack = read_data[3];

			ack++;
		}
		else
			ack = 200;
	}
	else
		ack = 0;

	return ack;
}

/**
  * @}
  * @}
  */
