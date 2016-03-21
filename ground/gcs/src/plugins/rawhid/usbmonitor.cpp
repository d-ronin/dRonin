/**
 ******************************************************************************
 *
 * @file       usbmonitor.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup RawHIDPlugin Raw HID Plugin
 * @{
 * @brief Implements the USB monitor on Mac using XXXXX
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


#include "usbmonitor.h"
#include <QDebug>

#include "rawhid.h"


//#define USB_MON_DEBUG
#ifdef USB_MON_DEBUG
#define USB_MON_QXTLOG_DEBUG(...) qDebug()<<__VA_ARGS__
#else  // USB_MON_DEBUG
#define USB_MON_QXTLOG_DEBUG(...)
#endif	// USB_MON_DEBUG

#define printf USB_MON_QXTLOG_DEBUG

USBMonitor *USBMonitor::m_instance = 0;

/**
  Initialize the USB monitor here
  */
USBMonitor::USBMonitor(QObject *parent) : QObject(parent) {
    m_instance = this;

    qRegisterMetaType<USBPortInfo>();

    hid_init();

    connect(&periodicTimer, SIGNAL(timeout()), this, SLOT(periodic()));
    periodicTimer.setSingleShot(true);
    periodicTimer.start(150);

    prevDevList = NULL;
}

USBMonitor::~USBMonitor()
{
    hid_free_enumeration(prevDevList);
}

void USBMonitor::periodic() {
    struct hid_device_info *hidDevList = hid_enumerate(0, 0, prevDevList);

    QList<USBPortInfo> unseenDevices = knowndevices;
    QList<USBPortInfo> newDevices;

    bool didAnything = false;

    for (struct hid_device_info *hidDev = hidDevList;
            hidDev != NULL;
            hidDev = hidDev->next) {
        USBPortInfo info;

        info.vendorID = hidDev->vendor_id;
        info.productID = hidDev->product_id;
        info.bcdDevice = hidDev->release_number;
        info.serialNumber = QString::fromWCharArray(hidDev->serial_number);
        info.product = QString::fromWCharArray(hidDev->product_string);
        info.manufacturer = QString::fromWCharArray(hidDev->manufacturer_string);
        if (!unseenDevices.removeOne(info)) {
            newDevices.append(info);
        } 
    }

    prevDevList = hidDevList;

    foreach (USBPortInfo item, unseenDevices) {
        didAnything = true;

        qDebug() << "Removing " << item.vendorID << item.productID << item.bcdDevice << item.serialNumber << item.product << item.manufacturer;

        knowndevices.removeOne(item);

        emit deviceRemoved(item);
    }

    foreach (USBPortInfo item, newDevices) {
        if (!knowndevices.contains(item)) {
            didAnything = true;

            qDebug() << "Adding " << item.vendorID << item.productID << item.bcdDevice << item.serialNumber << item.product << item.manufacturer;

            knowndevices.append(item);

            emit deviceDiscovered(item);
        }
    }

    if (didAnything) {
        qDebug() << "usbmonitor detection cycle complete.";
    }

    /* Ensure our signals are spaced out.  Also limit our CPU consumption */
    periodicTimer.start(150);
}

QList<USBPortInfo> USBMonitor::availableDevices()
{
    return knowndevices;
}

/**
  * @brief Be a bit more picky and ask only for a specific type of device:
  * @param[in] vid VID to screen or1 to ignore
  * @param[in] pid PID to screen or1 to ignore
  * @param[in] bcdDeviceMSB MSB of bcdDevice to screen or1 to ignore
  * @param[in] bcdDeviceLSB LSB of bcdDevice to screen or1 to ignore
  * @return List of USBPortInfo that meet this criterion
  * @note
  *   On OpenPilot, the bcdDeviceLSB indicates the run state: bootloader or running.
  *   bcdDeviceMSB indicates the board model.
  */
QList<USBPortInfo> USBMonitor::availableDevices(int vid, int pid, int bcdDeviceMSB, int bcdDeviceLSB)
{
    QList<USBPortInfo> thePortsWeWant;

    foreach (USBPortInfo port, knowndevices) {
        if((port.vendorID==vid || vid==-1) && (port.productID==pid || pid==-1) && ((port.bcdDevice>>8)==bcdDeviceMSB || bcdDeviceMSB==-1) &&
           ( (port.bcdDevice&0x00ff) ==bcdDeviceLSB || bcdDeviceLSB==-1))
            thePortsWeWant.append(port);
    }

    return thePortsWeWant;
}

USBMonitor* USBMonitor::instance()
{
    return m_instance;
}
