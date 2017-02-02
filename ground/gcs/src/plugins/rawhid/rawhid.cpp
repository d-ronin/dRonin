/**
 ******************************************************************************
 *
 * @file       rawhid.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup RawHIDPlugin Raw HID Plugin
 * @{
 * @brief Impliments a HID USB connection to the flight hardware as a QIODevice
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

#include "rawhid.h"

#include "rawhid_const.h"
#include "coreplugin/connectionmanager.h"
#include <extensionsystem/pluginmanager.h>
#include <QtGlobal>
#include <QList>
#include <QMutexLocker>

class IConnection;

//timeout value used when we want to return directly without waiting
static const int READ_TIMEOUT = 200;
static const int READ_SIZE = 64;

static const int WRITE_SIZE = 64;

static const int WRITE_RETRIES = 10;

RawHIDReadThread::RawHIDReadThread(hid_device *handle)
    : m_handle(handle),
      m_running(true)
{
}

RawHIDReadThread::~RawHIDReadThread()
{
    // This should already be done / is bogus

    m_running = false;
    //wait for the thread to terminate
    if(wait(10000) == false)
        qWarning() << "Cannot terminate RawHIDReadThread";
}

void RawHIDReadThread::run()
{
    while(m_running)
    {
        //here we use a temporary buffer so we don't need to lock
        //the mutex while we are reading from the device

        // Want to read in regular chunks that match the packet size the device
        // is using.  In this case it is 64 bytes (the interrupt packet limit)
        // although it would be nice if the device had a different report to
        // configure this
        unsigned char buffer[READ_SIZE] = {0};

        int ret = hid_read_timeout(m_handle, buffer, READ_SIZE, READ_TIMEOUT);

        if(ret > 0) //read some data
        {
            QMutexLocker lock(&m_readBufMtx);

            bool needSignal = m_readBuffer.size() == 0;

            // Note: Preprocess the USB packets in this OS independent code
            // First byte is report ID, second byte is the number of valid bytes
            m_readBuffer.append((char *) &buffer[2], buffer[1]);

            if (needSignal) {
                emit readyToRead();
            }
        }
        else if(ret == 0) //nothing read
        {
        }
        else // < 0 => error
        {
            // This thread exiting on error is OK/sane.
            m_running=false;
        }
    }
}

int RawHIDReadThread::getReadData(char *data, int size)
{
    QMutexLocker lock(&m_readBufMtx);

    size = qMin(size, m_readBuffer.size());

    memcpy(data, m_readBuffer.constData(), size);
    m_readBuffer.remove(0, size);

    return size;
}

qint64 RawHIDReadThread::getBytesAvailable()
{
    QMutexLocker lock(&m_readBufMtx);
    return m_readBuffer.size();
}

// *********************************************************************************

RawHIDWriteThread::RawHIDWriteThread(hid_device *handle)
    : m_handle(handle),
      m_running(true)
{
}

RawHIDWriteThread::~RawHIDWriteThread()
{
}

void RawHIDWriteThread::run()
{
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));

    int retry = 0;
    while(m_running)
    {
        unsigned char buffer[WRITE_SIZE] = {0};
        int size;

        {
            QMutexLocker lock(&m_writeBufMtx);
            size = qMin(WRITE_SIZE-2, m_writeBuffer.size());

            while(m_running && (size <= 0))
            {
                m_newDataToWrite.wait(&m_writeBufMtx);

                size = m_writeBuffer.size();
            }

            //NOTE: data size is limited to 2 bytes less than the
            //usb packet size (64 bytes for interrupt) to make room
            //for the reportID and valid data length
            size = qMin(WRITE_SIZE-2, m_writeBuffer.size());
            memcpy(&buffer[2], m_writeBuffer.constData(), size);

            buffer[0] = 2;    //reportID
            buffer[1] = size; //valid data length
        }

        // must hold lock through the send to know how much was sent
        int ret = hid_write(m_handle, buffer, WRITE_SIZE);

        if(ret > 0)
        {
            //only remove the size actually written to the device            
            QMutexLocker lock(&m_writeBufMtx);
            m_writeBuffer.remove(0, size);
        }
        else if(ret == -110) // timeout
        {
            // timeout occured
            RAW_HID_QXTLOG_DEBUG("Send Timeout: No data written to device.");
        }
        else if(ret < 0) // < 0 => error
        {
            ++retry;
            if (retry > WRITE_RETRIES)
            {
                retry = 0;
                qWarning() << "[RawHID] Error writing to device";
                break;          // Exit the loop but keep running.
            }
            else
            {
                this->msleep(40);
            }
        }
        else
        {
            RAW_HID_QXTLOG_DEBUG("No data written to device ??");
        }
    }

    while (m_running) {
        this->msleep(100);      // Wait until we've been asked to exit.
    }

    // If we're at this point, RawHID has asked us to exit, so the read thread is
    // down and no one will touch m_handle again.
    hid_close(m_handle);

    m_handle = NULL;
}

//! Tell the thread to stop and make sure it wakes up immediately
void RawHIDWriteThread::stop()
{
    m_running = false;

    QMutexLocker lock(&m_writeBufMtx);

    m_newDataToWrite.wakeOne();
}

int RawHIDWriteThread::pushDataToWrite(const char *data, int size)
{
    QMutexLocker lock(&m_writeBufMtx);

    m_writeBuffer.append(data, size);
    m_newDataToWrite.wakeOne(); //signal that new data arrived

    return size;
}

qint64 RawHIDWriteThread::getBytesToWrite()
{
    return m_writeBuffer.size();
}

// *********************************************************************************

RawHID::RawHID(USBDevice *deviceStructure)
    :QIODevice(),
    m_deviceInfo(deviceStructure),
    m_readThread(NULL),
    m_writeThread(NULL)
{
}

RawHID::~RawHID()
{
    hid_exit();
}


bool RawHID::open(OpenMode mode)
{
    hid_device *handle;

    // Initialize the hidapi library (safe to call multiple times)
    hid_init();

    // Open the device using the VID, PID
    handle = hid_open_path(m_deviceInfo->getPath().toLatin1());

    if (handle) {
        m_handle = handle;

        m_writeThread = new RawHIDWriteThread(m_handle);
        m_readThread = new RawHIDReadThread(m_handle);

        // Plumb through read thread's ready read signal to our clients
        connect(m_readThread, SIGNAL(readyToRead()), this, SLOT(sendReadyRead()));

        m_readThread->start();
        m_writeThread->start();
    } else {
        qWarning() << "[RawHID] Failed to open USB device";
        return false;
    }

    return QIODevice::open(mode);
}

void RawHID::sendReadyRead()
{
    emit readyRead();
}

void RawHID::close()
{
    if (!isOpen())
        return;

    RAW_HID_QXTLOG_DEBUG("RawHID: close()");

    m_readThread->stop();
    m_readThread->wait();       /* Block indefinitely for read thread exit */
    m_readThread->deleteLater();
    m_readThread = NULL;

    m_writeThread->stop();
    m_handle = NULL;

    m_writeThread = NULL;       /* Write thread is in charge of tearing itself and
                                 * m_handle down */

    QIODevice::close();
}

bool RawHID::isSequential() const
{
    return true;
}

qint64 RawHID::bytesAvailable() const
{
    if (!m_readThread)
        return -1;

    return m_readThread->getBytesAvailable() + QIODevice::bytesAvailable();
}

qint64 RawHID::bytesToWrite() const
{
    if (!m_writeThread)
        return -1;

    return m_writeThread->getBytesToWrite() + QIODevice::bytesToWrite();
}

qint64 RawHID::readData(char *data, qint64 maxSize)
{
    if (!m_readThread)
        return -1;

    return m_readThread->getReadData(data, maxSize);
}

qint64 RawHID::writeData(const char *data, qint64 maxSize)
{
    qint64 ret = m_writeThread->pushDataToWrite(data, maxSize);

    if (ret > 0) {
        emit bytesWritten(ret);
    }

    return ret;
}

/**
 * @}
 * @}
 */
