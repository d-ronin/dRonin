/**
 ******************************************************************************
 * @addtogroup FlightCore Core components
 * @{
 * @addtogroup UAVTalk UAVTalk implementation
 * @{
 *
 * @file       uavtalk.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @brief      Include file of the UAVTalk library
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

#ifndef UAVTALK_H
#define UAVTALK_H

// Public types
typedef int32_t (*UAVTalkOutputCb)(void *ctx, uint8_t *data, int32_t length);
typedef void (*UAVTalkAckCb)(void *ctx, uint32_t obj_id, uint16_t inst_id);
typedef void (*UAVTalkReqCb)(void *ctx, uint32_t obj_id, uint16_t inst_id);
typedef int32_t (*UAVTalkFileCb)(void *ctx, uint8_t *buf,
		uint32_t file_id, uint32_t offset, uint32_t len);

//! Tracking statistics for a UAVTalk connection
typedef struct {
	uint32_t txBytes;
	uint32_t rxBytes;
	uint32_t txObjectBytes;
	uint32_t rxObjectBytes;
	uint32_t rxObjects;
	uint32_t txObjects;
	uint32_t txErrors;
	uint32_t rxErrors;
} UAVTalkStats;

typedef void* UAVTalkConnection;

typedef enum {UAVTALK_STATE_ERROR = 0, UAVTALK_STATE_SYNC, UAVTALK_STATE_TYPE, UAVTALK_STATE_SIZE, UAVTALK_STATE_OBJID, UAVTALK_STATE_INSTID,
	      UAVTALK_STATE_DATA, UAVTALK_STATE_CS, UAVTALK_STATE_COMPLETE} UAVTalkRxState;

// Public functions
UAVTalkConnection UAVTalkInitialize(void *ctx, UAVTalkOutputCb outputStream, UAVTalkAckCb ackCallback, UAVTalkReqCb reqCallback, UAVTalkFileCb fileCallback);
int32_t UAVTalkSendObject(UAVTalkConnection connection, UAVObjHandle obj, uint16_t instId, uint8_t acked);
int32_t UAVTalkSendObjectTimestamped(UAVTalkConnection connectionHandle, UAVObjHandle obj, uint16_t instId);
int32_t UAVTalkSendNack(UAVTalkConnection connectionHandle, uint32_t objId);
void UAVTalkProcessInputStream(UAVTalkConnection connectionHandle, uint8_t *rxbytes,
		int numbytes);
UAVTalkRxState UAVTalkProcessInputStreamQuiet(UAVTalkConnection connection, uint8_t rxbyte);
UAVTalkRxState UAVTalkRelayInputStream(UAVTalkConnection connectionHandle, uint8_t rxbyte);
int32_t UAVTalkRelayPacket(UAVTalkConnection inConnectionHandle, UAVTalkConnection outConnectionHandle);
int32_t UAVTalkReceiveObject(UAVTalkConnection connectionHandle);
void UAVTalkGetStats(UAVTalkConnection connection, UAVTalkStats *stats);
uint32_t UAVTalkGetPacketObjId(UAVTalkConnection connection);
uint32_t UAVTalkGetPacketInstId(UAVTalkConnection connection);

#endif // UAVTALK_H
/**
 * @}
 * @}
 */
