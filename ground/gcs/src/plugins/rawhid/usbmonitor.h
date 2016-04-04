/**
 ******************************************************************************
 *
 * @file       usbmonitor.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://github.com/TauLabs, Copyright (C) 2013.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup RawHIDPlugin Raw HID Plugin
 * @{
 * @brief Monitors the USB bus for devince insertion/removal
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

#ifndef USBMONITOR_H
#define USBMONITOR_H

#include "rawhid_global.h"

#include <QThread>
#include <QTimer>
#include <QMutex>

struct USBPortInfo {
    QString serialNumber; // As a string as it can be anything, really...
    QString manufacturer;
    QString product;

    int vendorID;       ///< Vendor ID.
    int productID;      ///< Product ID
    int bcdDevice;

    unsigned char getRunState()
    {
        return bcdDevice&0x00ff;
    }

    bool operator==(USBPortInfo const &port)
    {
        if (port.vendorID != vendorID) return false;

        if (port.productID != productID) return false;

        if (port.bcdDevice != bcdDevice) return false;

        if (port.serialNumber != serialNumber) {
            if ((serialNumber != "") && (port.serialNumber != "")) {
                return false;
            }
        }

        /* Don't compare manufacturer or product strings for identification */

        return true;            // We ran the gauntlet and came out OK.
    }
};

Q_DECLARE_METATYPE(USBPortInfo);

/**
*   A monitor which will wait for device events.
*/

class RAWHID_EXPORT USBMonitor : public QObject
{
    Q_OBJECT

public:
    enum RunState {
        Bootloader = 0x01,
        Running = 0x02,
        Upgrader = 0x03,
    };

    static USBMonitor *instance();

    USBMonitor(QObject *parent = 0);
    ~USBMonitor();
    QList<USBPortInfo> availableDevices();
    QList<USBPortInfo> availableDevices(int vid, int pid, int boardModel, int runState);
signals:
    /*!
      A new device has been connected to the system.

      setUpNotifications() must be called first to enable event-driven device notifications.
      Currently only implemented on Windows and OS X.
      \param info The device that has been discovered.
    */
    void deviceDiscovered( const USBPortInfo &info );
    /*!
      A device has been disconnected from the system.

      setUpNotifications() must be called first to enable event-driven device notifications.
      Currently only implemented on Windows and OS X.
      \param info The device that was disconnected.
    */
    void deviceRemoved( const USBPortInfo &info );

private slots:
    void periodic();

private:
    //! List of known devices maintained by callbacks
    QList<USBPortInfo> knowndevices;

    Q_DISABLE_COPY(USBMonitor)
    static USBMonitor *m_instance;

    QTimer periodicTimer;

    struct hid_device_info *prevDevList;

};
#endif // USBMONITOR_H
