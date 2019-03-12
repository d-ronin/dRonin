/**
 ******************************************************************************
 * @file       uavtalk.cpp
 *
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVTalkPlugin UAVTalk Plugin
 * @{
 * @brief Implementation of the UAVTalk protocol which serializes and
 * deserializes UAVObjects
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

#include "uavtalk.h"
#include <QtEndian>
#include <QDebug>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/generalsettings.h>

// #define UAVTALK_DEBUG
#ifdef UAVTALK_DEBUG
#define UAVTALK_QXTLOG_DEBUG(...) qDebug() << __VA_ARGS__
#else // UAVTALK_DEBUG
#define UAVTALK_QXTLOG_DEBUG(...)
#endif // UAVTALK_DEBUG

#define SYNC_VAL 0x3C

const quint8 UAVTalk::crc_table[256] = {
    0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
    0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
    0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
    0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
    0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
    0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
    0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
    0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
    0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
    0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
    0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
    0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
    0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
    0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
    0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
    0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
};

/**
 * Constructor
 */
UAVTalk::UAVTalk(QIODevice *iodev, UAVObjectManager *objMngr, bool canBlock)
{
    io = iodev;

    this->objMngr = objMngr;
    this->canBlock = canBlock;

    startOffset = 0;
    filledBytes = 0;

    memset(&stats, 0, sizeof(ComStats));

    connect(io.data(), &QIODevice::readyRead, this, &UAVTalk::processInputStream);
}

UAVTalk::~UAVTalk()
{
    // According to Qt, it is not necessary to disconnect upon
    // object deletion.
    // disconnect(io, SIGNAL(readyRead()), this, SLOT(processInputStream()));
}

/**
 * Get the statistics counters
 */
UAVTalk::ComStats UAVTalk::getStats()
{
    UAVTalk::ComStats ret = stats;

    memset(&stats, 0, sizeof(ComStats));

    return ret;
}

/**
 * Called each time there are data in the input buffer
 */
void UAVTalk::processInputStream()
{
    while (io && io->isReadable()) {
        if (startOffset > (sizeof(rxBuffer) - MAX_PACKET_LENGTH)) {
            /* If we're not sure there's room for a frame, shift things left in
             * the buffer so that we can do a bigger read.
             */
            memmove(rxBuffer, rxBuffer + startOffset, filledBytes - startOffset);

            filledBytes -= startOffset;
            startOffset = 0;
        }

        int bytes = io->read(reinterpret_cast<char *>(rxBuffer + filledBytes),
                             sizeof(rxBuffer) - filledBytes);

        if (bytes <= 0) {
            return;
        }

        filledBytes += bytes;
        stats.rxBytes += bytes;

        while (processInput());
    }
}

/**
 * Request an update for the specified object, on success the object data would have been
 * updated by the GCS.
 * \param[in] obj Object to update
 * \param[in] allInstances If set true then all instances will be updated
 * \return Success (true), Failure (false)
 */
bool UAVTalk::sendObjectRequest(UAVObject *obj, bool allInstances)
{
    return objectTransaction(obj, TYPE_OBJ_REQ, allInstances);
}

/**
 * Send the specified object through the telemetry link.
 * \param[in] obj Object to send
 * \param[in] acked Selects if an ack is required
 * \param[in] allInstances If set true then all instances will be updated
 * \return Success (true), Failure (false)
 */
bool UAVTalk::sendObject(UAVObject *obj, bool acked, bool allInstances)
{
    if (acked) {
        return objectTransaction(obj, TYPE_OBJ_ACK, allInstances);
    } else {
        return objectTransaction(obj, TYPE_OBJ, allInstances);
    }
}

/**
 * Execute the requested transaction on an object.
 * \param[in] obj Object
 * \param[in] type Transaction type
 * 			  TYPE_OBJ: send object with no ack,
 * 			  TYPE_OBJ_REQ: request object update
 * 			  TYPE_OBJ_ACK: send object with an ack
 * \param[in] allInstances If set true then all instances will be updated
 * \return Success (true), Failure (false)
 */
bool UAVTalk::objectTransaction(UAVObject *obj, quint8 type, bool allInstances)
{
    if (type == TYPE_OBJ_ACK || type == TYPE_OBJ_REQ || type == TYPE_OBJ) {
        return transmitObject(obj, type, allInstances);
    } else {
        return false;
    }
}

/**
 * Processes a frame containing file data.
 * \param fielId the received file id
 * \param data Buffer to the file data header
 * \param length Number of bytes of file data header + file data.
 */
bool UAVTalk::receiveFileChunk(quint32 fileId, quint8 *data, quint32 length)
{
    UAVTalkFileData *hdr = reinterpret_cast<UAVTalkFileData *>(data);

    if (length < sizeof(*hdr)) {
        return false;
    }

    data += sizeof(*hdr);
    length -= sizeof(*hdr);

    // qDebug() << "Received file chunk, file=" << fileId << ", offset = " <<
    //    hdr->offset << ", len=" << length << ", flags=" << hdr->flags;

    emit fileDataReceived(fileId, hdr->offset, data, length, !!(hdr->flags & FILEDATA_FLAG_EOF),
                          !!(hdr->flags & FILEDATA_FLAG_LAST));

    return true;
}

/**
 * Process a frame from input, if available.
 * \return False if there was insufficient data for a frame, true if trying
 * again is worthwhile.
 */
bool UAVTalk::processInput()
{
    unsigned int bytesAvail = filledBytes - startOffset;

    if (bytesAvail < sizeof(UAVTalkHeader)) {
        return false;
    }

    UAVTalkHeader *hdr = reinterpret_cast<UAVTalkHeader *>(rxBuffer + startOffset);

    /* Basic framing checks.  If these fail, skip forward one byte and retry
     * to capture stream sync.
     */
    if (hdr->sync != SYNC_VAL) {
        startOffset++;
        stats.rxErrors++;

        return true;
    }

    if ((hdr->type & VER_MASK) != TYPE_VER) {
        startOffset++;
        stats.rxErrors++;

        return true;
    }

    if (hdr->size < sizeof(UAVTalkHeader)) {
        startOffset++;
        stats.rxErrors++;

        return true;
    }

    /* OK, let's ensure we have enough bytes for the whole frame.
     * Size doesn't include CRC, so add one.
     */

    if ((hdr->size + 1u) > bytesAvail) {
        return false;
    }

    quint8 ourCrc = updateCRC(0, rxBuffer + startOffset, hdr->size);
    quint8 *theirCrc = rxBuffer + startOffset + hdr->size;

    if (ourCrc != *theirCrc) {
        /* Since we can't trust hdr->size for sure, we should just skip
         * forward one byte.
         */

        startOffset++;
        stats.rxErrors++;

        return true;
    }

    quint8 *payload = rxBuffer + startOffset + sizeof(*hdr);
    unsigned int payloadBytes = hdr->size - sizeof(*hdr);

    /* At this point, we'll advance startOffset for the entire length of
     * frame, and not touch startOffset again this function!
     */
    startOffset += hdr->size + 1;

    /* OK, we have a complete frame as encoded on the wire.  Time to do things
     * with it.
     */
    quint8 rxType = hdr->type & TYPE_MASK;

    quint32 rxObjId = qFromLittleEndian(hdr->objId);

    if (rxType == TYPE_FILEDATA) {
        return receiveFileChunk(rxObjId, payload, payloadBytes);
    }

    UAVObject *rxObj = objMngr->getObject(rxObjId);

    if (rxObj == nullptr) {
        stats.rxErrors++;
        UAVTALK_QXTLOG_DEBUG("UAVTalk: unknown object");

        if (rxType == TYPE_OBJ_REQ || rxType == TYPE_OBJ_ACK) {
            UAVTALK_QXTLOG_DEBUG("UAVTalk: (transmitting NACK)");
            transmitNack(rxObjId);
        }

        return true;
    }

    quint16 rxInstId = 0;

    if (!rxObj->isSingleInstance()) {
        if ((rxType != TYPE_NACK) || (payloadBytes == 2)) {
            /* Receiving the instid is optional on an nack-- can just mean
             * "nack everything" */
            rxInstId = *(payload++);
            rxInstId |= *(payload++) << 8;

            payloadBytes -= 2;
        }
    }

    /* XXX timestamps */

    // Check data length
    if (rxType == TYPE_OBJ_REQ || rxType == TYPE_ACK || rxType == TYPE_NACK) {
        if (payloadBytes != 0) {
            UAVTALK_QXTLOG_DEBUG("UAVTalk: Unexpected data in req/ack/nack");
            stats.rxErrors++;

            return true;
        }
    } else {
        if (payloadBytes != rxObj->getNumBytes()) {
            UAVTALK_QXTLOG_DEBUG("UAVTalk: Unexpected payload size for obj");
            stats.rxErrors++;

            return true;
        }
    }

    receiveObject(rxType, rxObjId, rxInstId, payload, payloadBytes);
    stats.rxObjectBytes += payloadBytes;
    stats.rxObjects++;

    // Done
    return true;
}

/**
 * Receive an object. This function process objects received through the telemetry stream.
 * \param[in] type Type of received message (TYPE_OBJ, TYPE_OBJ_REQ, TYPE_OBJ_ACK, TYPE_ACK,
 * TYPE_NACK)
 * \param[in] obj Handle of the received object
 * \param[in] instId The instance ID of UAVOBJ_ALL_INSTANCES for all instances.
 * \param[in] data Data buffer
 * \param[in] length Buffer length
 * \return Success (true), Failure (false)
 */
bool UAVTalk::receiveObject(quint8 type, quint32 objId, quint16 instId, quint8 *data,
                            quint32 length)
{
    Q_UNUSED(length);
    UAVObject *obj = nullptr;
    bool error = false;
    bool allInstances = (instId == ALL_INSTANCES);

    // Process message type
    switch (type) {
    case TYPE_OBJ: // We have received an object.
        // All instances, not allowed for OBJ messages
        if (!allInstances) {
            // Get object and update its data
            obj = updateObject(objId, instId, data);
            if (obj == nullptr) {
                UAVTALK_QXTLOG_DEBUG(
                    QString("[uavtalk.cpp  ] Received a UAVObject update for a UAVObject we don't "
                            "know about OBJID:%0 INSTID:%1")
                        .arg(QString(QString("0x") + QString::number(objId, 16).toUpper()))
                        .arg(instId));
                error = true;
            }
        } else {
            error = true;
        }
        break;
    case TYPE_OBJ_ACK: // We have received an object and are asked for an ACK
        // All instances, not allowed for OBJ_ACK messages
        if (!allInstances) {
            // Get object and update its data
            obj = updateObject(objId, instId, data);
            // Transmit ACK
            if (obj != nullptr) {
                transmitObject(obj, TYPE_ACK, false);
            } else {
                UAVTALK_QXTLOG_DEBUG(QString("[uavtalk.cpp  ] Received an acknowledged UAVObject "
                                             "update for a UAVObject we don't know about:")
                                         .arg(obj->getName()));
                transmitNack(objId);
                error = true;
            }
        } else {
            error = true;
        }
        break;
    case TYPE_OBJ_REQ: // We are being asked for an object
        // Get object, if all instances are requested get instance 0 of the object
        if (allInstances) {
            obj = objMngr->getObject(objId);
        } else {
            obj = objMngr->getObject(objId, instId);
        }
        // If object was found transmit it
        if (obj != nullptr) {
            transmitObject(obj, TYPE_OBJ, allInstances);
        } else {
            // Object was not found, transmit a NACK with the
            // objId which was not found.
            transmitNack(objId);
            error = true;
        }
        break;
    case TYPE_NACK: // We have received a NACK for an object that does not exist on the remote end.
        // (but should exist on our end)
        // All instances, if nacked, are substantially the same as the first
        // inst being nack'd.
        if (allInstances) {
            instId = 0;
        }

        // Get object
        obj = objMngr->getObject(objId, instId);
        // Check if object exists:
        if (obj != nullptr) {
            UAVTALK_QXTLOG_DEBUG(
                QString("[uavtalk.cpp  ] The %0 UAVObject does not exist on the remote end, "
                        "got a Nack")
                    .arg(obj->getName()
                         + QString(QString(" 0x") + QString::number(objId, 16).toUpper())));
            emit nackReceived(obj);
        } else {
            UAVTALK_QXTLOG_DEBUG(
                QString("[uavtalk.cpp  ] Critical error: Received a Nack for an unknown "
                        "UAVObject:%0")
                    .arg(QString(QString("0x") + QString::number(objId, 16).toUpper())));
            error = true;
        }
        break;
    case TYPE_ACK: // We have received a ACK, supposedly after sending an object with OBJ_ACK
        // All instances, not allowed for ACK messages
        if (!allInstances) {
            // Get object
            obj = objMngr->getObject(objId, instId);
            UAVTALK_QXTLOG_DEBUG(
                QString("[uavtalk.cpp  ] Got ack for instance:%0 of UAVObject:%1 with ID:%2")
                    .arg(instId)
                    .arg(obj->getName())
                    .arg(QString(QString("0x") + QString::number(objId, 16).toUpper())));
            // Check if we actually know this object (tiny chance the ObjID
            // could be unknown and got through CRC check...)
            if (obj != nullptr) {
                UAVTALK_QXTLOG_DEBUG(
                    QString("[uavtalk.cpp  ] UAVObject name:%0").arg(obj->getName()));
                emit ackReceived(obj);
            } else {
                error = true;
            }
        }
        break;
    default:
        error = true;
    }
    // Done (exit value is "success", hence the "!" below)
    return !error;
}

/**
 * Update the data of an object from a byte array (unpack).
 * If the object instance could not be found in the list, then a
 * new one is created.
 */
UAVObject *UAVTalk::updateObject(quint32 objId, quint16 instId, quint8 *data)
{
    // Get object
    UAVObject *obj = objMngr->getObject(objId, instId);
    // If the instance does not exist create it
    if (obj == nullptr) {
        // Get the object type
        UAVObject *tobj = objMngr->getObject(objId);
        if (tobj == nullptr) {
            return nullptr;
        }
        // Make sure this is a data object
        UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(tobj);
        if (dobj == nullptr) {
            return nullptr;
        }
        // Create a new instance, unpack and register
        UAVDataObject *instobj = dobj->clone(instId);
        if (!objMngr->registerObject(instobj)) {
            return nullptr;
        }
        instobj->unpack(data);
        return instobj;
    } else {
        // Unpack data into object instance
        obj->unpack(data);
        return obj;
    }
}

/**
 * Send an object through the telemetry link.
 * \param[in] obj Object to send
 * \param[in] type Transaction type
 * \param[in] allInstances True is all instances of the object are to be sent
 * \return Success (true), Failure (false)
 */
bool UAVTalk::transmitObject(UAVObject *obj, quint8 type, bool allInstances)
{
    // If all instances are requested on a single instance object it is an error
    if (allInstances && obj->isSingleInstance()) {
        allInstances = false;
    }

    // Process message type
    if (type == TYPE_OBJ || type == TYPE_OBJ_ACK) {
        if (allInstances) {
            // Get number of instances
            quint32 numInst = objMngr->getNumInstances(obj->getObjID());
            // Send all instances
            for (quint32 instId = 0; instId < numInst; ++instId) {
                UAVObject *inst = objMngr->getObject(obj->getObjID(), instId);
                transmitSingleObject(inst, type, false);
            }
            return true;
        } else {
            return transmitSingleObject(obj, type, false);
        }
    } else if (type == TYPE_OBJ_REQ) {
        return transmitSingleObject(obj, TYPE_OBJ_REQ, allInstances);
    } else if (type == TYPE_ACK) {
        if (!allInstances) {
            return transmitSingleObject(obj, TYPE_ACK, false);
        } else {
            return false;
        }
    } else {
        return false;
    }
}

/**
 * Transmit a NACK through the telemetry link.
 * This method is separate from transmitsingleobject because we are
 * using an objectID for an unknown object.
 * \param[in] objId the ObjectID we rejected
 */
bool UAVTalk::transmitNack(quint32 objId)
{
    txBuffer[0] = SYNC_VAL;
    txBuffer[1] = TYPE_VER | TYPE_NACK;

    qToLittleEndian<quint32>(objId, &txBuffer[4]);

    return transmitFrame(8, false);
}

/**
 * Perform the final work of transmitting a frame from the txBuffer.
 * \param[in] length Frame length, not including checksum.
 * \param[in] incrTxObj Boolean, default true, wheter to update tx object
 * counters.
 */
bool UAVTalk::transmitFrame(quint32 length, bool incrTxObj)
{
    if (length >= 256) {
        Q_ASSERT(0);
        return false;
    }

    txBuffer[2] = length;
    txBuffer[3] = 0;

    txBuffer[length] = updateCRC(0, txBuffer, length);

    // QFileDevice, used for saving logs, has quite a large buffer, we're not worried about it blocking
    bool blocked = canBlock && io->bytesToWrite() >= TX_BACKLOG_SIZE;

    if (!io.isNull() && io->isWritable() && !blocked) {
        io->write(reinterpret_cast<const char *>(txBuffer), length + CHECKSUM_LENGTH);
    } else {
        UAVTALK_QXTLOG_DEBUG("UAVTalk: TX refused");
        ++stats.txErrors;
        return false;
    }

    // Update stats
    if (incrTxObj) {
        ++stats.txObjects;
        /* This can end up counting instance IDs as object length, but big
         * woop / I think this is arguably correct.
         */
        stats.txObjectBytes += length - MIN_HEADER_LENGTH;
    }

    stats.txBytes += length + CHECKSUM_LENGTH;

    return true;
}

/**
 * Send a request for file data.
 * \param[in] fileId The file id to request.
 * \param[in] offset The first requested chunk of the file.
 */
bool UAVTalk::requestFile(quint32 fileId, quint32 offset)
{
    txBuffer[0] = SYNC_VAL;
    txBuffer[1] = TYPE_VER | TYPE_FILEREQ;
    qToLittleEndian<quint32>(fileId, &txBuffer[4]);
    qToLittleEndian<quint32>(offset, &txBuffer[8]);
    txBuffer[12] = 0;
    txBuffer[13] = 0;

    // qDebug() << "Sent file req offs=" << offset;

    return transmitFrame(14);
}

/**
 * Send an object through the telemetry link.
 * \param[in] obj Object handle to send
 * \param[in] type Transaction type
 * \return Success (true), Failure (false)
 */
bool UAVTalk::transmitSingleObject(UAVObject *obj, quint8 type, bool allInstances)
{
    qint32 length;
    qint32 dataOffset;
    quint32 objId;
    quint16 instId;
    quint16 allInstId = ALL_INSTANCES;

    // Setup type and object id fields
    objId = obj->getObjID();
    txBuffer[0] = SYNC_VAL;
    txBuffer[1] = TYPE_VER | type;
    qToLittleEndian<quint32>(objId, &txBuffer[4]);

    // Setup instance ID if one is required
    if (obj->isSingleInstance()) {
        dataOffset = 8;
    } else {
        // Check if all instances are requested
        if (allInstances) {
            qToLittleEndian<quint16>(allInstId, &txBuffer[8]);
        } else {
            instId = obj->getInstID();
            qToLittleEndian<quint16>(instId, &txBuffer[8]);
        }
        dataOffset = 10;
    }

    // Determine data length
    if (type == TYPE_OBJ_REQ || type == TYPE_ACK) {
        length = 0;
    } else {
        length = obj->getNumBytes();
    }

    // Check length
    if (length >= MAX_PAYLOAD_LENGTH) {
        return false;
    }

    // Copy data (if any)
    if (length > 0) {
        if (!obj->pack(&txBuffer[dataOffset])) {
            return false;
        }
    }

    return transmitFrame(dataOffset + length);
}

/**
 * Update the crc value with new data.
 *
 * Generated by pycrc v0.7.5, http://www.tty1.net/pycrc/
 * using the configuration:
 *    Width        = 8
 *    Poly         = 0x07
 *    XorIn        = 0x00
 *    ReflectIn    = False
 *    XorOut       = 0x00
 *    ReflectOut   = False
 *    Algorithm    = table-driven
 *
 * \param crc      The current crc value.
 * \param data     Pointer to a buffer of \a data_len bytes.
 * \param length   Number of bytes in the \a data buffer.
 * \return         The updated crc value.
 */

quint8 UAVTalk::updateCRC(quint8 crc, const quint8 *data, qint32 length)
{
    while (length--)
        crc = crc_table[crc ^ *data++];
    return crc;
}
