/**
 ******************************************************************************
 *
 * @file       rawhid.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
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

#ifndef RAWHID_H
#define RAWHID_H

#include "rawhid_global.h"

#include <QByteArray>
#include <QIODevice>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include <coreplugin/iconnection.h>

#include "hidapi/hidapi.h"
#include "usbmonitor.h"
#include "usbdevice.h"

//#define RAW_HID_DEBUG
#ifdef RAW_HID_DEBUG
#define RAW_HID_QXTLOG_DEBUG(...) qDebug()<<__VA_ARGS__
#else  // RAW_HID_DEBUG
#define RAW_HID_QXTLOG_DEBUG(...)
#endif	// RAW_HID_DEBUG

/**
*   Thread to desynchronize reading from the device
*/
class RawHIDReadThread : public QThread
{
    Q_OBJECT

public:
    RawHIDReadThread(hid_device *device);
    virtual ~RawHIDReadThread();

    /** Return the data read so far without waiting */
    int getReadData(char *data, int size);

    /** return the bytes buffered */
    qint64 getBytesAvailable();

    void stop() { m_running = false; }

signals:
    void readyToRead();

protected:
    void run();

    /** QByteArray might not be the most efficient way to implement
    a circular buffer but it's good enough and very simple */
    QByteArray m_readBuffer;

    /** A mutex to protect read buffer */
    QMutex m_readBufMtx;

    hid_device *m_handle;

    bool m_running;
};


// *********************************************************************************

/**
*  This class is nearly the same than RawHIDReadThread but for writing
*/
class RawHIDWriteThread : public QThread
{
    Q_OBJECT

public:
    RawHIDWriteThread(hid_device *device);
    virtual ~RawHIDWriteThread();

    /** Add some data to be written without waiting */
    int pushDataToWrite(const char *data, int size);

    /** Return the number of bytes buffered */
    qint64 getBytesToWrite();

    void stop();

protected:
    void run();

    /** QByteArray might not be the most efficient way to implement
    a circular buffer but it's good enough and very simple */
    QByteArray m_writeBuffer;

    /** A mutex to protect read buffer */
    QMutex m_writeBufMtx;

    /** Synchronize task with data arival */
    QWaitCondition m_newDataToWrite;

    hid_device *m_handle;

    bool m_running;
}; 

// *********************************************************************************

/**
*   The actual IO device that will be used to communicate
*   with the board.
*/
class RAWHID_EXPORT RawHID : public QIODevice
{
	Q_OBJECT

public:
    RawHID();
    RawHID(USBDevice *deviceName);
    virtual ~RawHID();

    virtual bool open(OpenMode mode);
    virtual void close();
    virtual bool isSequential() const;

private slots:
    void sendReadyRead();

protected:
    virtual qint64 readData(char *data, qint64 maxSize);
    virtual qint64 writeData(const char *data, qint64 maxSize);
    virtual qint64 bytesAvailable() const;
    virtual qint64 bytesToWrite() const;

    USBDevice *m_deviceInfo;
    hid_device *m_handle;

    RawHIDReadThread *m_readThread;
    RawHIDWriteThread *m_writeThread;
};

#endif // RAWHID_H
