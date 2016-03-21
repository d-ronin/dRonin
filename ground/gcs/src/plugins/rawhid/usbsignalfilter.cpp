/**
 ******************************************************************************
 *
 * @file       usbmonitor_mac.cpp
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

#include "usbsignalfilter.h"
#include <QDebug>

//#define USB_FILTER_DEBUG
#ifdef USB_FILTER_DEBUG
#define USB_FILTER_QXTLOG_DEBUG(...) qDebug()<<__VA_ARGS__
#else  // USB_FILTER_DEBUG
#define USB_FILTER_QXTLOG_DEBUG(...)
#endif	// USB_FILTER_DEBUG

bool USBSignalFilter::portMatches(USBPortInfo port)
{
    if ((m_vid.contains(port.vendorID) || m_vid.isEmpty()) &&
            (port.productID==m_pid || m_pid==-1) &&
            ((port.bcdDevice>>8)==m_boardModel || m_boardModel==-1) &&
            (port.getRunState() == m_runState || m_runState==-1)) {
        return true;
    }

    return false;
}

void USBSignalFilter::m_deviceDiscovered(USBPortInfo port)
{
    if (portMatches(port)) {
        USB_FILTER_QXTLOG_DEBUG("USBSignalFilter emit device discovered");
        emit deviceDiscovered();
    }
}

void USBSignalFilter::m_deviceRemoved(USBPortInfo port)
{
    if (portMatches(port)) {
        USB_FILTER_QXTLOG_DEBUG("USBSignalFilter emit device removed");
        emit deviceRemoved();
    }
}

USBSignalFilter::USBSignalFilter(int vid, int pid, int boardModel, int runState):
    m_pid(pid),
    m_boardModel(boardModel),
    m_runState(runState)
{
    if (vid != -1)
        m_vid.append(vid);

    connect(USBMonitor::instance(), SIGNAL(deviceDiscovered(USBPortInfo)), this, SLOT(m_deviceDiscovered(USBPortInfo)), Qt::QueuedConnection);
    connect(USBMonitor::instance(), SIGNAL(deviceRemoved(USBPortInfo)), this, SLOT(m_deviceRemoved(USBPortInfo)), Qt::QueuedConnection);
}

USBSignalFilter::USBSignalFilter(QList<int> vid, int pid, int boardModel, int runState):
    m_vid(vid),
    m_pid(pid),
    m_boardModel(boardModel),
    m_runState(runState)
{
    connect(USBMonitor::instance(), SIGNAL(deviceDiscovered(USBPortInfo)), this, SLOT(m_deviceDiscovered(USBPortInfo)), Qt::QueuedConnection);
    connect(USBMonitor::instance(), SIGNAL(deviceRemoved(USBPortInfo)), this, SLOT(m_deviceRemoved(USBPortInfo)), Qt::QueuedConnection);
}
