/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{ 
 * @addtogroup GimbalControl GimbalControl Module
 * @{ 
 *
 * @file       gimbalcontrol.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @brief      Receive CameraDesired via the CAN bus
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
#include "modulesettings.h"
#include "pios_can.h"

#include "attitudeactual.h"
#include "brushlessgimbalsettings.h"
#include "cameradesired.h"

#include "pios_thread.h"
#include "pios_queue.h"

// Private constants
#define STACK_SIZE_BYTES 512
#define TASK_PRIORITY PIOS_THREAD_PRIO_LOW

// Private types

// Private variables
static struct pios_queue *queue_gimbal_cmd;
static struct pios_queue *queue_roll_pitch;
static struct pios_thread *gimbalControlTaskHandle;
static bool module_enabled;

// Private functions
static void    gimbalControlTask(void *parameters);

// External variables
extern uintptr_t pios_can_id;

/**
 * Initialise the UAVORelay module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t GimbalControlInitialize(void)
{

#ifdef MODULE_GimbalControl_BUILTIN
	module_enabled = true;
#else
	module_enabled = false;
#endif

	if (!module_enabled)
		return -1;

	// Create object queues
	queue_gimbal_cmd = PIOS_CAN_RegisterMessageQueue(pios_can_id, PIOS_CAN_GIMBAL);
	queue_roll_pitch = PIOS_CAN_RegisterMessageQueue(pios_can_id, PIOS_CAN_ATTITUDE_ROLL_PITCH);

	BrushlessGimbalSettingsInitialize();
	CameraDesiredInitialize();
	AttitudeActualInitialize();


	return 0;
}

/**
 * Start the UAVORelay module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t GimbalControlStart(void)
{
	//Check if module is enabled or not
	if (module_enabled == false) {
		return -1;
	}
	
	// Start relay task
	gimbalControlTaskHandle = PIOS_Thread_Create(
			gimbalControlTask, "GimbalControl", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);

	TaskMonitorAdd(TASKINFO_RUNNING_UAVORELAY, gimbalControlTaskHandle);
	
	return 0;
}

MODULE_INITCALL(GimbalControlInitialize, GimbalControlStart)
;

static void gimbalControlTask(void *parameters)
{

	// Loop forever
	while (1) {

		struct pios_can_gimbal_message bgc_message;
		struct pios_can_roll_pitch_message roll_pitch_message;

		// Wait for queue message
		if (PIOS_Queue_Receive(queue_gimbal_cmd, &bgc_message, 5) == true) {
			CameraDesiredData cameraDesired;
			CameraDesiredGet(&cameraDesired);

			cameraDesired.Declination = bgc_message.setpoint_pitch;
			cameraDesired.Bearing = bgc_message.setpoint_yaw;
			cameraDesired.Roll = bgc_message.fc_roll;
			cameraDesired.Pitch = bgc_message.fc_pitch;
			cameraDesired.Yaw = bgc_message.fc_yaw;
			CameraDesiredSet(&cameraDesired);
		} else if (PIOS_Queue_Receive(queue_roll_pitch, &roll_pitch_message, 0) == true) {
			AttitudeActualData attitudeActual;
			AttitudeActualGet(&attitudeActual);
			attitudeActual.Roll = roll_pitch_message.fc_roll;
			attitudeActual.Pitch = roll_pitch_message.fc_pitch;
			AttitudeActualSet(&attitudeActual);
		}

	}
}


/**
  * @}
  * @}
  */
