/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup FirmwareIAPModule Read the firmware IAP values
 * @{
 *
 * @file       firmwareiap.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @brief      In Application Programming module to support firmware upgrades by
 *             providing a means to enter the bootloader.
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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */
#include <stdint.h>

#include "pios.h"
#include <pios_board_info.h>
#include "openpilot.h"
#include <eventdispatcher.h>
#include "firmwareiap.h"
#include "firmwareiapobj.h"
#include "flightstatus.h"
#include "pios_servo.h"
#include "pios_thread.h"

// Private constants
#define IAP_CMD_STEP_1      1122
#define IAP_CMD_STEP_2      2233
#define IAP_CMD_STEP_3      3344
#define IAP_CMD_STEP_3NB    4455

#define IAP_CMD_CRC         100
#define IAP_CMD_VERIFY      101
#define IAP_CMD_VERSION	    102

#define IAP_STATE_READY     0
#define IAP_STATE_STEP_1    1
#define IAP_STATE_STEP_2    2
#define IAP_STATE_RESETTING 3

#define RESET_DELAY_MS         500 /* delay before sending reset to INS */

const uint32_t iap_time_2_low_end = 500;
const uint32_t iap_time_2_high_end = 5000;
const uint32_t iap_time_3_low_end = 500;
const uint32_t iap_time_3_high_end = 5000;

// Private types

// Private variables
static uint8_t reset_count = 0;
static uint32_t lastResetSysTime;
static uint8_t board_rev = 0;

// Private functions
static void FirmwareIAPCallback(UAVObjEvent* ev, void *ctx, void *obj, int len);

static uint32_t	get_time(void);

static void resetTask(UAVObjEvent * ev, void *ctx, void *obj, int len);

/**
 * Initialise the module, called on startup.
 * \returns 0 on success or -1 if initialisation failed
 */

/*!
 * \brief   Performs object initialization functions.
 * \param   None.
 * \return  0 - under all cases
 *
 * \note
 *
 */
MODULE_HIPRI_INITCALL(FirmwareIAPInitialize, 0)
int32_t FirmwareIAPInitialize()
{
	
	if (FirmwareIAPObjInitialize() == -1) {
		return -1;
	}
	
	const struct pios_board_info * bdinfo = &pios_board_info_blob;

	if (!board_rev)
		board_rev = bdinfo->board_rev;

	FirmwareIAPObjData data;
	FirmwareIAPObjGet(&data);

	data.BoardType= bdinfo->board_type;
	PIOS_BL_HELPER_FLASH_Read_Description(data.Description,FIRMWAREIAPOBJ_DESCRIPTION_NUMELEM);
	PIOS_SYS_SerialNumberGetBinary(data.CPUSerial);
	data.BoardRevision = board_rev;
	data.crc = PIOS_BL_HELPER_CRC_Memory_Calc();
	FirmwareIAPObjSet( &data );
	if(bdinfo->magic==PIOS_BOARD_INFO_BLOB_MAGIC) FirmwareIAPObjConnectCallback( &FirmwareIAPCallback );
	return 0;
}

/*!
 * \brief	FirmwareIAPCallback - callback function for firmware IAP requests
 * \param[in]  ev - pointer objevent
 * \retval	None.
 *
 * \note
 *
 */
static uint8_t    iap_state = IAP_STATE_READY;
static void FirmwareIAPCallback(UAVObjEvent* ev, void *ctx, void *obj, int len)
{
	(void) ctx; (void) obj; (void) len;

	static uint32_t   last_time = 0;
	uint32_t          this_time;
	uint32_t          delta;
	
	if(iap_state == IAP_STATE_RESETTING)
		return;

	FirmwareIAPObjData data;
	FirmwareIAPObjGet(&data);
	
	if ( ev->obj == FirmwareIAPObjHandle() )	{
		// Get the input object data
		FirmwareIAPObjGet(&data);
		this_time = get_time();
		delta = this_time - last_time;
		last_time = this_time;

		switch(iap_state) {
			case IAP_STATE_READY:
				if( data.Command == IAP_CMD_STEP_1 ) {
					iap_state = IAP_STATE_STEP_1;
				}            break;
			case IAP_STATE_STEP_1:
				if( data.Command == IAP_CMD_STEP_2 ) {
					if( delta > iap_time_2_low_end && delta < iap_time_2_high_end ) {
						iap_state = IAP_STATE_STEP_2;
					} else {
						iap_state = IAP_STATE_READY;
					}
				} else {
					iap_state = IAP_STATE_READY;
				}
				break;
			case IAP_STATE_STEP_2:
				if( (data.Command == IAP_CMD_STEP_3) || (data.Command == IAP_CMD_STEP_3NB) ) {
					if( delta > iap_time_3_low_end && delta < iap_time_3_high_end ) {
						
						FlightStatusData flightStatus;
						FlightStatusGet(&flightStatus);
						
						if(flightStatus.Armed != FLIGHTSTATUS_ARMED_DISARMED) {
							// Abort any attempts if not disarmed
							iap_state = IAP_STATE_READY;
							break;
						}
							
						// we've met the three sequence of command numbers
						// we've met the time requirements.
						if(data.Command == IAP_CMD_STEP_3) {
							PIOS_IAP_SetRequest1();
							PIOS_IAP_SetRequest2();
						}
						else if(data.Command == IAP_CMD_STEP_3NB) {
							PIOS_IAP_SetRequest1();
							PIOS_IAP_SetRequest3();
						}
						
						/* Note: Cant just wait timeout value, because first time is randomized */
						reset_count = 0;
						lastResetSysTime = PIOS_Thread_Systime();
						UAVObjEvent * ev = PIOS_malloc_no_dma(sizeof(UAVObjEvent));
						memset(ev,0,sizeof(UAVObjEvent));
						EventPeriodicCallbackCreate(ev, resetTask, 100);
						PIOS_Servo_PrepareForReset(); // Stop PWM
						iap_state = IAP_STATE_RESETTING;
					} else {
						iap_state = IAP_STATE_READY;
					}
				} else {
					iap_state = IAP_STATE_READY;
				}
				break;
			case IAP_STATE_RESETTING:
				// stay here permanentally, should reboot
				break;
			default:
				iap_state = IAP_STATE_READY;
				last_time = 0; // Reset the time counter, as we are not doing a IAP reset
				break;
		}
	}
}



// Returns number of milliseconds from the start of the kernel.

/*!
 * \brief   Returns number of milliseconds from the start of the kernel
 * \param   None.
 * \return  number of milliseconds from the start of the kernel.
 *
 * \note
 *
 */

static uint32_t get_time(void)
{
	return PIOS_Thread_Systime();
}

/**
 * Executed by event dispatcher callback to reset INS before resetting OP 
 */
static void resetTask(UAVObjEvent * ev, void *ctx, void *obj, int len)
{
	(void) ctx; (void) obj; (void) len;

#if defined(PIOS_INCLUDE_ANNUNC) && defined(PIOS_LED_HEARTBEAT)
	PIOS_ANNUNC_Toggle(PIOS_LED_HEARTBEAT);
#endif	/* PIOS_LED_HEARTBEAT */

#if defined(PIOS_INCLUDE_ANNUNC) && defined(PIOS_LED_ALARM)
	PIOS_ANNUNC_Toggle(PIOS_LED_ALARM);
#endif	/* PIOS_LED_ALARM */

	FirmwareIAPObjData data;
	FirmwareIAPObjGet(&data);

	if((uint32_t) (PIOS_Thread_Systime() - lastResetSysTime) > RESET_DELAY_MS) {
		lastResetSysTime = PIOS_Thread_Systime();
			PIOS_SYS_Reset();
	}
}

int32_t FirmwareIAPStart()
{
	return 0;
};

void FirmwareIAPSetBoardRev(uint8_t rev)
{
	board_rev = rev;
}

/**
 * @}
 * @}
 */
