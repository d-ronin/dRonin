/**
 ******************************************************************************
 * @addtogroup FlightCore Core components
 * @{
 * @addtogroup UAVTalk UAVTalk implementation
 * @{
 *
 * @file       uavtalk.c
 *
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @author     dRonin, http://dRonin.org, Copyright (C) 2017
 *
 * @brief      UAVTalk library, implements to telemetry protocol. See the wiki for more details.
 *
 * This code packetizes UAVObjects into UAVTalk messages includes the CRC for
 * transmission through various physical layers.
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

#include "openpilot.h"
#include "uavtalk.h"
#include "uavtalk_priv.h"
#include "pios_mutex.h"
#include "pios_thread.h"

// Private functions
static int32_t objectTransaction(UAVTalkConnectionData *connection, UAVObjHandle objectId, uint16_t instId, uint8_t type);
static int32_t sendObject(UAVTalkConnectionData *connection, UAVObjHandle obj, uint16_t instId, uint8_t type);
static int32_t sendSingleObject(UAVTalkConnectionData *connection, UAVObjHandle obj, uint16_t instId, uint8_t type);
static int32_t receiveObject(UAVTalkConnectionData *connection);
static int32_t sendNack(UAVTalkConnectionData *connection, uint32_t objId, uint16_t instId);

/**
 * Initialize the UAVTalk library
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] outputStream Function pointer that is called to send a data buffer
 * \param[in] ackCallback A function to invoke when receiving an ack
 * \parampin] fileCallback A function which provides file data upon demand
 * \return 0 Success
 * \return -1 Failure
 */
UAVTalkConnection UAVTalkInitialize(void *ctx, UAVTalkOutputCb outputStream,
		UAVTalkAckCb ackCallback, UAVTalkReqCb reqCallback,
		UAVTalkFileCb fileCallback)
{
	// allocate object
	UAVTalkConnectionData * connection = PIOS_malloc_no_dma(sizeof(UAVTalkConnectionData));
	if (!connection) return 0;

	*connection = (UAVTalkConnectionData) {
		.canari = UAVTALK_CANARI,
		.iproc = { .state = UAVTALK_STATE_SYNC, },
		.cbCtx = ctx,
		.outCb = outputStream,
		.ackCb = ackCallback,
		.reqCb = reqCallback,
		.fileCb = fileCallback,
	};

	connection->lock = PIOS_Recursive_Mutex_Create();
	PIOS_Assert(connection->lock != NULL);
	// allocate buffers
	connection->rxBuffer = PIOS_malloc(UAVTALK_MAX_PACKET_LENGTH);
	if (!connection->rxBuffer) return 0;
	connection->txBuffer = PIOS_malloc(UAVTALK_MAX_PACKET_LENGTH);
	if (!connection->txBuffer) return 0;

	return (UAVTalkConnection) connection;
}

/**
 * Get communication statistics counters since last call (reset afterwards)
 * \param[in] connection UAVTalkConnection to be used
 * @param[out] statsOut Statistics counters
 */
void UAVTalkGetStats(UAVTalkConnection connectionHandle, UAVTalkStats* statsOut)
{
	UAVTalkConnectionData *connection;
	CHECKCONHANDLE(connectionHandle,connection,return );

	// Lock
	PIOS_Recursive_Mutex_Lock(connection->lock, PIOS_MUTEX_TIMEOUT_MAX);

	// Copy stats
	memcpy(statsOut, &connection->stats, sizeof(UAVTalkStats));

	// And reset.
	memset(&connection->stats, 0, sizeof(UAVTalkStats));

	// Release lock
	PIOS_Recursive_Mutex_Unlock(connection->lock);
}

/**
 * Send the specified object through the telemetry link.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] obj Object to send
 * \param[in] instId The instance ID or UAVOBJ_ALL_INSTANCES for all instances.
 * \param[in] acked True if an ack is requested
 * \return 0 Success
 * \return -1 Failure
 */
int32_t UAVTalkSendObject(UAVTalkConnection connectionHandle, UAVObjHandle obj, uint16_t instId, uint8_t acked)
{
	UAVTalkConnectionData *connection;
	CHECKCONHANDLE(connectionHandle,connection,return -1);
	// Send object
	if (acked) {
		return objectTransaction(connection, obj, instId, UAVTALK_TYPE_OBJ_ACK);
	} else {
		return objectTransaction(connection, obj, instId, UAVTALK_TYPE_OBJ);
	}
}

/**
 * Send the specified object through the telemetry link with a timestamp.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] obj Object to send
 * \param[in] instId The instance ID or UAVOBJ_ALL_INSTANCES for all instances.
 * \param[in] acked Selects if an ack is required (1:ack required, 0: ack not required)
 * \param[in] timeoutMs Time to wait for the ack, when zero it will return immediately
 * \return 0 Success
 * \return -1 Failure
 */
int32_t UAVTalkSendObjectTimestamped(UAVTalkConnection connectionHandle, UAVObjHandle obj, uint16_t instId)
{
	UAVTalkConnectionData *connection;
	CHECKCONHANDLE(connectionHandle,connection,return -1);
	// Send object
	return objectTransaction(connection, obj, instId, UAVTALK_TYPE_OBJ_TS);
}

/**
 * Execute the requested transaction on an object.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] obj Object
 * \param[in] instId The instance ID of UAVOBJ_ALL_INSTANCES for all instances.
 * \param[in] type Transaction type
 *                        UAVTALK_TYPE_OBJ: send object,
 *                        UAVTALK_TYPE_OBJ_REQ: request object update
 *                        UAVTALK_TYPE_OBJ_ACK: send object with an ack
 * \return 0 Success
 * \return -1 Failure
 */
static int32_t objectTransaction(UAVTalkConnectionData *connection, UAVObjHandle obj, uint16_t instId, uint8_t type)
{
	if (type == UAVTALK_TYPE_OBJ || type == UAVTALK_TYPE_OBJ_TS ||
			type == UAVTALK_TYPE_OBJ_ACK) {
		sendObject(connection, obj, instId, type);

		return 0;
	} else {
		return -1;
	}
}

/**
 * Process an byte from the telemetry stream.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] rxbyte Received byte
 * \return UAVTalkRxState
 */
UAVTalkRxState UAVTalkProcessInputStreamQuiet(UAVTalkConnection connectionHandle, uint8_t rxbyte)
{
	UAVTalkConnectionData *connection;
	CHECKCONHANDLE(connectionHandle,connection,return -1);

	UAVTalkInputProcessor *iproc = &connection->iproc;
	++connection->stats.rxBytes;

	if (iproc->state == UAVTALK_STATE_ERROR) {
		connection->stats.rxErrors++;
		iproc->state = UAVTALK_STATE_SYNC;
	} else if (iproc->state == UAVTALK_STATE_COMPLETE) {
		iproc->state = UAVTALK_STATE_SYNC;
	}

	if (iproc->rxPacketLength < 0xffff)
		iproc->rxPacketLength++;   // update packet byte count

	// Receive state machine
	switch (iproc->state)
	{
	case UAVTALK_STATE_SYNC:
		if (rxbyte != UAVTALK_SYNC_VAL)
			break;

		// Initialize and update the CRC
		iproc->cs = PIOS_CRC_updateByte(0, rxbyte);

		iproc->rxPacketLength = 1;

		iproc->state = UAVTALK_STATE_TYPE;
		break;

	case UAVTALK_STATE_TYPE:

		// update the CRC
		iproc->cs = PIOS_CRC_updateByte(iproc->cs, rxbyte);

		if ((rxbyte & UAVTALK_TYPE_MASK) != UAVTALK_TYPE_VER) {
			iproc->state = UAVTALK_STATE_ERROR;
			break;
		}

		iproc->type = rxbyte;

		iproc->packet_size = 0;

		iproc->state = UAVTALK_STATE_SIZE;
		iproc->rxCount = 0;
		break;

	case UAVTALK_STATE_SIZE:

		// update the CRC
		iproc->cs = PIOS_CRC_updateByte(iproc->cs, rxbyte);

		if (iproc->rxCount == 0) {
			iproc->packet_size += rxbyte;
			iproc->rxCount++;
			break;
		}

		iproc->packet_size += rxbyte << 8;

		if (iproc->packet_size < UAVTALK_MIN_HEADER_LENGTH ||
				iproc->packet_size > UAVTALK_MAX_HEADER_LENGTH + UAVTALK_MAX_PAYLOAD_LENGTH) { // incorrect packet size
			iproc->state = UAVTALK_STATE_ERROR;
			break;
		}

		iproc->rxCount = 0;
		iproc->objId = 0;
		iproc->state = UAVTALK_STATE_OBJID;
		break;

	case UAVTALK_STATE_OBJID:

		// update the CRC
		iproc->cs = PIOS_CRC_updateByte(iproc->cs, rxbyte);

		iproc->objId += rxbyte << (8*(iproc->rxCount++));

		if (iproc->rxCount < 4)
			break;

		if (iproc->type == UAVTALK_TYPE_FILEREQ) {
			/* Slightly overloaded from "normal" case.  Consume
			 * 4 bytes of offset and 2 bytes of flags.
			 */

			iproc->instanceLength = 0;
			iproc->rxCount = 0;
			iproc->length = 6;

			if ((iproc->packet_size - iproc->rxPacketLength) !=
					iproc->length) {
				iproc->state = UAVTALK_STATE_ERROR;
				break;
			}

			iproc->state = UAVTALK_STATE_DATA;

			break;
		}

		// Search for object.
		iproc->obj = UAVObjGetByID(iproc->objId);

		// Determine data length
		if (iproc->type == UAVTALK_TYPE_OBJ_REQ || iproc->type == UAVTALK_TYPE_ACK || iproc->type == UAVTALK_TYPE_NACK) {
			iproc->length = 0;
			iproc->instanceLength = 0;

			/* Length is always pretty much expected to be 0
			 * here, but it can be 2 if it's a multiple inst
			 * obj requested.  Don't peer into metadata to
			 * figure this out-- use the packet length
			 * [so we can properly NAK objects we don't know]
			 */
			if ((iproc->packet_size - iproc->rxPacketLength) == 2) {
				iproc->instanceLength = 2;
			} else if (iproc->length > 0) {
				iproc->state = UAVTALK_STATE_ERROR;
				break; 
			}
		} else {
			if (iproc->obj) {
				iproc->length = UAVObjGetNumBytes(iproc->obj);
				iproc->instanceLength = (UAVObjIsSingleInstance(iproc->obj) ? 0 : 2);
			} else {
				// We don't know if it's a multi-instance object, so just assume it's 0.
				iproc->instanceLength = 0;
				iproc->length = iproc->packet_size - iproc->rxPacketLength;
			}
		}

		// Check length and determine next state
		if (iproc->length >= UAVTALK_MAX_PAYLOAD_LENGTH) {
			iproc->state = UAVTALK_STATE_ERROR;
			break;
		}

		// Check the lengths match
		if ((iproc->rxPacketLength + iproc->instanceLength + iproc->length) != iproc->packet_size) { // packet error - mismatched packet size
			if (iproc->instanceLength == 0) {
				// Try again with a 2 inst len
				// to accept LP's fork of
				// protocol.
				iproc->instanceLength = 2;
			}
		}

		if ((iproc->rxPacketLength + iproc->instanceLength + iproc->length) != iproc->packet_size) { // packet error - mismatched packet size
			iproc->state = UAVTALK_STATE_ERROR;
			break;
		}

		iproc->instId = 0;
		if (iproc->type == UAVTALK_TYPE_NACK) {
			// If this is a NACK, we skip to Checksum
			iproc->state = UAVTALK_STATE_CS;
		}
		// Check if this is a single instance object (i.e. if the instance ID field is coming next)
		else if (iproc->instanceLength) {
			iproc->state = UAVTALK_STATE_INSTID;
		} else {
			// If there is a payload get it, otherwise receive checksum
			if (iproc->length > 0)
				iproc->state = UAVTALK_STATE_DATA;
			else
				iproc->state = UAVTALK_STATE_CS;
		}
		iproc->rxCount = 0;

		break;

	case UAVTALK_STATE_INSTID:

		// update the CRC
		iproc->cs = PIOS_CRC_updateByte(iproc->cs, rxbyte);

		iproc->instId += rxbyte << (8*(iproc->rxCount++));

		if (iproc->rxCount < 2)
			break;

		iproc->rxCount = 0;

		// If there is a payload get it, otherwise receive checksum
		if (iproc->length > 0)
			iproc->state = UAVTALK_STATE_DATA;
		else
			iproc->state = UAVTALK_STATE_CS;

		break;

	case UAVTALK_STATE_DATA:

		// update the CRC
		iproc->cs = PIOS_CRC_updateByte(iproc->cs, rxbyte);

		connection->rxBuffer[iproc->rxCount++] = rxbyte;
		if (iproc->rxCount < iproc->length)
			break;

		iproc->state = UAVTALK_STATE_CS;
		iproc->rxCount = 0;
		break;

	case UAVTALK_STATE_CS:

		// the CRC byte
		if (rxbyte != iproc->cs) { // packet error - faulty CRC
			connection->stats.rxCRC++;
			iproc->state = UAVTALK_STATE_ERROR;
			break;
		}

		if (iproc->rxPacketLength != (iproc->packet_size + 1)) { // packet error - mismatched packet size
			iproc->state = UAVTALK_STATE_ERROR;
			break;
		}

		connection->stats.rxObjectBytes += iproc->length;
		connection->stats.rxObjects++;

		iproc->state = UAVTALK_STATE_COMPLETE;
		break;

	default:
		iproc->state = UAVTALK_STATE_ERROR;
	}

	// Done
	return iproc->state;
}

/**
 * Process an byte from the telemetry stream.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] rxbyte Received byte
 * \return UAVTalkRxState
 */
void UAVTalkProcessInputStream(UAVTalkConnection connectionHandle, uint8_t *rxbytes,
		int numbytes)
{
	UAVTalkConnectionData *connection;

	CHECKCONHANDLE(connectionHandle,connection,return);

	for (int i = 0; i < numbytes; i++) {
		UAVTalkRxState state =
			UAVTalkProcessInputStreamQuiet(connectionHandle,
					rxbytes[i]);

		if (state == UAVTALK_STATE_COMPLETE) {
			receiveObject(connection);
		}
	}
}

/**
 * Send a parsed packet received on one connection handle out on a different connection handle.
 * The packet must be in a complete state, meaning it is completed parsing.
 * The packet is re-assembled from the component parts into a complete message and sent.
 * This can be used to relay packets from one UAVTalk connection to another.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] rxbyte Received byte
 * \return 0 Success
 * \return -1 Failure
 */
int32_t UAVTalkRelayPacket(UAVTalkConnection inConnectionHandle, UAVTalkConnection outConnectionHandle)
{
	UAVTalkConnectionData *inConnection;

	CHECKCONHANDLE(inConnectionHandle, inConnection, return -1);
	UAVTalkInputProcessor *inIproc = &inConnection->iproc;

	// The input packet must be completely parsed.
	if (inIproc->state != UAVTALK_STATE_COMPLETE) {
		return -1;
	}

	UAVTalkConnectionData *outConnection;
	CHECKCONHANDLE(outConnectionHandle, outConnection, return -1);

	if (!outConnection->outCb) {
		outConnection->stats.txErrors++;

		return -1;
	}

	// Lock
	PIOS_Recursive_Mutex_Lock(outConnection->lock, PIOS_MUTEX_TIMEOUT_MAX);

	outConnection->txBuffer[0] = UAVTALK_SYNC_VAL;
	// Setup type
	outConnection->txBuffer[1] = inIproc->type;
	// next 2 bytes are reserved for data length (inserted here later)
	// Setup object ID
	outConnection->txBuffer[4] = (uint8_t)(inIproc->objId & 0xFF);
	outConnection->txBuffer[5] = (uint8_t)((inIproc->objId >> 8) & 0xFF);
	outConnection->txBuffer[6] = (uint8_t)((inIproc->objId >> 16) & 0xFF);
	outConnection->txBuffer[7] = (uint8_t)((inIproc->objId >> 24) & 0xFF);
	int32_t headerLength = 8;

	if (inIproc->instanceLength) {
		// Setup instance ID
		outConnection->txBuffer[8] = (uint8_t)(inIproc->instId & 0xFF);
		outConnection->txBuffer[9] = (uint8_t)((inIproc->instId >> 8) & 0xFF);
		headerLength = 10;
	}

	// Copy data (if any)
	if (inIproc->length > 0) {
		memcpy(&outConnection->txBuffer[headerLength], inConnection->rxBuffer, inIproc->length);
	}

	// Store the packet length
	outConnection->txBuffer[2] = (uint8_t)((headerLength + inIproc->length) & 0xFF);
	outConnection->txBuffer[3] = (uint8_t)(((headerLength + inIproc->length) >> 8) & 0xFF);

	// Copy the checksum
	outConnection->txBuffer[headerLength + inIproc->length] = inIproc->cs;

	// Send the buffer.
	int32_t rc = (*outConnection->outCb)(outConnection->cbCtx, outConnection->txBuffer, headerLength + inIproc->length + UAVTALK_CHECKSUM_LENGTH);

	// Update stats
	outConnection->stats.txBytes += (rc > 0) ? rc : 0;

	// evaluate return value before releasing the lock
	int32_t ret = 0;
	if (rc != (int32_t)(headerLength + inIproc->length + UAVTALK_CHECKSUM_LENGTH)) {
		outConnection->stats.txErrors++;
		ret = -1;
	}

	// Release lock
	PIOS_Recursive_Mutex_Unlock(outConnection->lock);

	// Done
	return ret;
}

/**
 * Complete receiving a UAVTalk packet.  This will cause the packet to be unpacked, acked, etc.
 * \param[in] connectionHandle UAVTalkConnection to be used
 * \return 0 Success
 * \return -1 Failure
 */
int32_t UAVTalkReceiveObject(UAVTalkConnection connectionHandle)
{
	UAVTalkConnectionData *connection;

	CHECKCONHANDLE(connectionHandle, connection, return -1);

	UAVTalkInputProcessor *iproc = &connection->iproc;
	if (iproc->state != UAVTALK_STATE_COMPLETE) {
		return -1;
	}

	return receiveObject(connection);
}

/**
 * Get the object ID of the current packet.
 * \param[in] connectionHandle UAVTalkConnection to be used
 * \return The object ID, or 0 on error.
 */
uint32_t UAVTalkGetPacketObjId(UAVTalkConnection connectionHandle)
{
	UAVTalkConnectionData *connection;

	CHECKCONHANDLE(connectionHandle, connection, return 0);

	return connection->iproc.objId;
}

/**
 * Get the instance ID of the current packet.
 * \param[in] connectionHandle UAVTalkConnection to be used
 * \return The instance ID, or 0 on error.
 */
uint32_t UAVTalkGetPacketInstId(UAVTalkConnection connectionHandle)
{
	UAVTalkConnectionData *connection;

	CHECKCONHANDLE(connectionHandle, connection, return 0);

	return connection->iproc.instId;
}

/**
 * Handles a request for file data.
 * \param[in] connection The connection on which a request was just received.
 */
static void handleFileReq(UAVTalkConnectionData *connection)
{
	UAVTalkInputProcessor *iproc = &connection->iproc;
	uint32_t file_id = iproc->objId;

	struct filereq_data *req = (struct filereq_data *) connection->rxBuffer;

	/* printf("Got filereq for file_id=%08x offs=%d\n", file_id, req->offset); */

	/* Need txbuffer, and need to make sure file response msgs
	 * are contiguous on link.
	 */
	PIOS_Recursive_Mutex_Lock(connection->lock, PIOS_MUTEX_TIMEOUT_MAX);

	connection->txBuffer[0] = UAVTALK_SYNC_VAL;  // sync byte
	connection->txBuffer[1] = UAVTALK_TYPE_FILEDATA;
	// data length inserted here below
	connection->txBuffer[4] = (uint8_t)(file_id & 0xFF);
	connection->txBuffer[5] = (uint8_t)((file_id >> 8) & 0xFF);
	connection->txBuffer[6] = (uint8_t)((file_id >> 16) & 0xFF);
	connection->txBuffer[7] = (uint8_t)((file_id >> 24) & 0xFF);

	uint8_t data_offs = 8;

	struct fileresp_data *resp =
		(struct fileresp_data *) (connection->txBuffer + data_offs);

	data_offs += sizeof(*resp);

	uint32_t file_offset = req->offset;

	for (int i = 0; ; i++) {
		resp->offset = file_offset;
		resp->flags = 0;

		int32_t cb_numbytes = -1;

		if (connection->fileCb) {
			cb_numbytes = connection->fileCb(connection->cbCtx,
				connection->txBuffer + data_offs,
				file_id, file_offset, 100);
		}

		uint8_t total_len = data_offs;

		if (cb_numbytes > 0) {
			total_len += cb_numbytes;

			file_offset += cb_numbytes;

			if (i == 5) {	/* 6 messages per chunk */
				resp->flags = UAVTALK_FILEDATA_LAST;
			} else {
				resp->flags = 0;
			}
		} else {
			/* End of file, last chunk in sequence */
			resp->flags = UAVTALK_FILEDATA_LAST |
				UAVTALK_FILEDATA_EOF;
		}

		// Store the packet length
		connection->txBuffer[2] = (uint8_t)((total_len) & 0xFF);
		connection->txBuffer[3] = (uint8_t)(((total_len) >> 8) & 0xFF);

		// Calculate checksum
		connection->txBuffer[total_len] = PIOS_CRC_updateCRC(0,
				connection->txBuffer, total_len);

		int32_t rc = (*connection->outCb)(connection->cbCtx,
				connection->txBuffer, total_len + 1);

		if (rc == total_len) {
			// Update stats
			connection->stats.txBytes += total_len;
		}

		if (resp->flags & UAVTALK_FILEDATA_LAST) {
			break;
		}
	}

	PIOS_Recursive_Mutex_Unlock(connection->lock);
}

/**
 * Receive an object. This function process objects received through the telemetry stream.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] type Type of received message (UAVTALK_TYPE_OBJ, UAVTALK_TYPE_OBJ_REQ, UAVTALK_TYPE_OBJ_ACK, UAVTALK_TYPE_ACK, UAVTALK_TYPE_NACK)
 * \param[in] objId ID of the object to work on
 * \param[in] instId The instance ID of UAVOBJ_ALL_INSTANCES for all instances.
 * \param[in] data Data buffer
 * \param[in] length Buffer length
 * \return 0 Success
 * \return -1 Failure
 */
static int32_t receiveObject(UAVTalkConnectionData *connection)
{
	int32_t ret = 0;

	UAVTalkInputProcessor *iproc = &connection->iproc;

	uint8_t type = iproc->type;
	uint32_t objId = iproc->objId;
	uint16_t instId = iproc->instId;

	UAVObjHandle obj = iproc->obj;

	/* Handle ACK/NACK --- don't bother to look up IDs etc for these
	 * because we don't need it.
	 *
	 * Treat NACKs and ACKs identically -- ground has done all it wants
	 * with them.  This incurs a small penalty from calling the callback
	 * when GCS is a mismatched version and likes NACKing us, but ensures
	 * that we don't start blocking the telemetry session for an ACK that
	 * will never come.
	 */

	if ((type == UAVTALK_TYPE_NACK) || (type == UAVTALK_TYPE_ACK)) {
		if (connection->ackCb) {
			connection->ackCb(connection->cbCtx, objId, instId);
		}

		return 0;
	} else if (type == UAVTALK_TYPE_OBJ_REQ) {
		if (connection->reqCb) {
			connection->reqCb(connection->cbCtx, objId, instId);
			return 0;
		}

		return -1;
	}

	/* File request data is a special case. */
	if (type == UAVTALK_TYPE_FILEREQ) {
		handleFileReq(connection);

		return 0;
	}

	PIOS_Recursive_Mutex_Lock(connection->lock, PIOS_MUTEX_TIMEOUT_MAX);

	// Process message type
	switch (type) {
	case UAVTALK_TYPE_OBJ:
		// All instances, not allowed for OBJ messages
		if (obj && (instId != UAVOBJ_ALL_INSTANCES)) {
			// Unpack object, if the instance does not exist it will be created!
			UAVObjUnpack(obj, instId, connection->rxBuffer);
		} else {
			ret = -1;
		}
		break;
	case UAVTALK_TYPE_OBJ_ACK:
		// All instances, not allowed for OBJ_ACK messages
		if (obj && (instId != UAVOBJ_ALL_INSTANCES)) {
			// Unpack object, if the instance does not exist it will be created!
			if (UAVObjUnpack(obj, instId, connection->rxBuffer) == 0) {
				// Transmit ACK
				sendObject(connection, obj, instId, UAVTALK_TYPE_ACK);
			} else {
				ret = -1;
			}
		} else {
			// We don't know this object, and we complain about it
			sendNack(connection, objId, 0);
			ret = -1;
		}
		break;
	default:
		ret = -1;
	}

	PIOS_Recursive_Mutex_Unlock(connection->lock);

	// Done
	return ret;
}

/**
 * Send an object through the telemetry link.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] obj Object handle to send
 * \param[in] instId The instance ID or UAVOBJ_ALL_INSTANCES for all instances
 * \param[in] type Transaction type
 * \return 0 Success
 * \return -1 Failure
 */
static int32_t sendObject(UAVTalkConnectionData *connection, UAVObjHandle obj, uint16_t instId, uint8_t type)
{
	uint32_t numInst;
	uint32_t n;

	// If all instances are requested and this is a single instance object, force instance ID to zero
	if (instId == UAVOBJ_ALL_INSTANCES && UAVObjIsSingleInstance(obj) ) {
		instId = 0;
	}

	// Process message type
	// XXX some of these combinations don't make sense, like obj with ack
	// requested for all instances [because if we implemented it properly
	// it would be -exceptionally- costly]
	if (type == UAVTALK_TYPE_OBJ || type == UAVTALK_TYPE_OBJ_TS ||
			type == UAVTALK_TYPE_OBJ_ACK) {
		if (instId == UAVOBJ_ALL_INSTANCES) {
			// Get number of instances
			numInst = UAVObjGetNumInstances(obj);
			// Send all instances
			for (n = 0; n < numInst; ++n) {
				sendSingleObject(connection, obj, n, type);
			}
			return 0;
		} else {
			return sendSingleObject(connection, obj, instId, type);
		}
	} else if (type == UAVTALK_TYPE_OBJ_REQ) {
		return sendSingleObject(connection, obj, instId, UAVTALK_TYPE_OBJ_REQ);
	} else if (type == UAVTALK_TYPE_ACK) {
		if (instId != UAVOBJ_ALL_INSTANCES) {
			return sendSingleObject(connection, obj, instId, UAVTALK_TYPE_ACK);
		} else {
			return -1;
		}
	} else {
		return -1;
	}
}

/**
 * Send an object through the telemetry link.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] obj Object handle to send
 * \param[in] instId The instance ID (can NOT be UAVOBJ_ALL_INSTANCES, use sendObject() instead)
 * \param[in] type Transaction type
 * \return 0 Success
 * \return -1 Failure
 */
static int32_t sendSingleObject(UAVTalkConnectionData *connection, UAVObjHandle obj, uint16_t instId, uint8_t type)
{
	int32_t length;
	int32_t dataOffset;
	uint32_t objId;

	if (!connection->outCb) return -1;

	// Determine data length
	if (type == UAVTALK_TYPE_OBJ_REQ || type == UAVTALK_TYPE_ACK) {
		length = 0;
	} else {
		length = UAVObjGetNumBytes(obj);
	}

	// Check length
	if (length >= UAVTALK_MAX_PAYLOAD_LENGTH) {
		return -1;
	}

	// Setup type and object id fields
	objId = UAVObjGetID(obj);

	PIOS_Recursive_Mutex_Lock(connection->lock, PIOS_MUTEX_TIMEOUT_MAX);

	connection->txBuffer[0] = UAVTALK_SYNC_VAL;  // sync byte
	connection->txBuffer[1] = type;
	// data length inserted here below
	connection->txBuffer[4] = (uint8_t)(objId & 0xFF);
	connection->txBuffer[5] = (uint8_t)((objId >> 8) & 0xFF);
	connection->txBuffer[6] = (uint8_t)((objId >> 16) & 0xFF);
	connection->txBuffer[7] = (uint8_t)((objId >> 24) & 0xFF);

	// Setup instance ID if one is required
	if (UAVObjIsSingleInstance(obj)) {
		dataOffset = 8;
	} else {
		connection->txBuffer[8] = (uint8_t)(instId & 0xFF);
		connection->txBuffer[9] = (uint8_t)((instId >> 8) & 0xFF);
		dataOffset = 10;
	}

	// Add timestamp when the transaction type is appropriate
	if (type & UAVTALK_TIMESTAMPED) {
		uint32_t time = PIOS_Thread_Systime();
		connection->txBuffer[dataOffset] = (uint8_t)(time & 0xFF);
		connection->txBuffer[dataOffset + 1] = (uint8_t)((time >> 8) & 0xFF);
		dataOffset += 2;
	}

	// Copy data (if any)
	if (length > 0) {
		if (UAVObjPack(obj, instId, &connection->txBuffer[dataOffset]) < 0) {
			PIOS_Recursive_Mutex_Unlock(connection->lock);
			return -1;
		}
	}

	// Store the packet length
	connection->txBuffer[2] = (uint8_t)((dataOffset+length) & 0xFF);
	connection->txBuffer[3] = (uint8_t)(((dataOffset+length) >> 8) & 0xFF);

	// Calculate checksum
	connection->txBuffer[dataOffset+length] = PIOS_CRC_updateCRC(0, connection->txBuffer, dataOffset+length);

	uint16_t tx_msg_len = dataOffset+length+UAVTALK_CHECKSUM_LENGTH;
	int32_t rc = (*connection->outCb)(connection->cbCtx, connection->txBuffer,
			tx_msg_len);

	if (rc == tx_msg_len) {
		// Update stats
		++connection->stats.txObjects;
		connection->stats.txBytes += tx_msg_len;
		connection->stats.txObjectBytes += length;
	}

	// Done
	PIOS_Recursive_Mutex_Unlock(connection->lock);
	return 0;
}

/**
 * Send a NACK through the telemetry link.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] objId Object ID to send a NACK for
 * \param[in] instId inst ID to send a NACK for-- 0 implies "all"
 * \return 0 Success
 * \return -1 Failure
 */
int32_t UAVTalkSendNack(UAVTalkConnection connectionHandle, uint32_t objId,
		uint16_t instId)
{
	UAVTalkConnectionData *connection;
	CHECKCONHANDLE(connectionHandle, connection, return -1);

	return sendNack(connection, objId, instId);
}

static int32_t sendNack(UAVTalkConnectionData *connection, uint32_t objId,
		uint16_t instId)
{
	int32_t dataOffset;

	if (!connection->outCb) return -1;

	PIOS_Recursive_Mutex_Lock(connection->lock, PIOS_MUTEX_TIMEOUT_MAX);
	connection->txBuffer[0] = UAVTALK_SYNC_VAL;  // sync byte
	connection->txBuffer[1] = UAVTALK_TYPE_NACK;
	// data length inserted here below
	connection->txBuffer[4] = (uint8_t)(objId & 0xFF);
	connection->txBuffer[5] = (uint8_t)((objId >> 8) & 0xFF);
	connection->txBuffer[6] = (uint8_t)((objId >> 16) & 0xFF);
	connection->txBuffer[7] = (uint8_t)((objId >> 24) & 0xFF);

	dataOffset = 8;

	if (instId) {
		dataOffset += 2;

		connection->txBuffer[8] = (uint8_t)(instId & 0xFF);
		connection->txBuffer[9] = (uint8_t)((instId >> 8) & 0xFF);
	}

	// Store the packet length
	connection->txBuffer[2] = (uint8_t)((dataOffset) & 0xFF);
	connection->txBuffer[3] = (uint8_t)(((dataOffset) >> 8) & 0xFF);

	// Calculate checksum
	connection->txBuffer[dataOffset] = PIOS_CRC_updateCRC(0, connection->txBuffer, dataOffset);

	uint16_t tx_msg_len = dataOffset + UAVTALK_CHECKSUM_LENGTH;
	int32_t rc = (*connection->outCb)(connection->cbCtx, connection->txBuffer,
			tx_msg_len);

	if (rc == tx_msg_len) {
		// Update stats
		connection->stats.txBytes += tx_msg_len;
	}

	PIOS_Recursive_Mutex_Unlock(connection->lock);

	// Done
	return 0;
}

/**
 * @}
 * @}
 */
