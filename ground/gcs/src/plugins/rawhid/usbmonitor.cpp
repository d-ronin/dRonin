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

USBMonitor* USBMonitor::m_instance = 0;

/**
  Initialize the USB monitor here
  */
USBMonitor::USBMonitor(QObject *parent) : QObject(parent) {
    m_instance = this;

    hid_init();

    connect(&periodicTimer, SIGNAL(timeout()), this, SLOT(periodic()));
    periodicTimer.start(200);
}

USBMonitor::~USBMonitor()
{
}

void USBMonitor::periodic() {
    struct hid_device_info *hid_dev_list = hid_enumerate(0, 0);

    QList<USBPortInfo> unseenDevices = knowndevices;
    QList<USBPortInfo> newDevices;

    for (struct hid_device_info *hid_dev = hid_dev_list;
            hid_dev != NULL;
            hid_dev = hid_dev->next) {
        USBPortInfo info;

        info.vendorID = hid_dev->vendor_id;
        info.productID = hid_dev->product_id;
        info.bcdDevice = hid_dev->release_number;
        info.serialNumber = QString::fromWCharArray(hid_dev->serial_number);
        info.product = QString::fromWCharArray(hid_dev->product_string);
        info.manufacturer = QString::fromWCharArray(hid_dev->manufacturer_string);

        if (!unseenDevices.removeOne(info)) {
            newDevices.append(info);
        } 
    }

    hid_free_enumeration(hid_dev_list);

    foreach (USBPortInfo item, unseenDevices) {
        qDebug() << "Removing " << item.vendorID << item.productID << item.bcdDevice << item.serialNumber << item.product << item.manufacturer;

        knowndevices.removeOne(item);

        emit deviceRemoved(item);
    }

    foreach (USBPortInfo item, newDevices) {
        qDebug() << "Adding " << item.vendorID << item.productID << item.bcdDevice << item.serialNumber << item.product << item.manufacturer;

        knowndevices.append(item);

        emit deviceDiscovered(item);
    }
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
