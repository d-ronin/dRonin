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

#ifndef USBSIGNALFILTER_H
#define USBSIGNALFILTER_H
#include <QObject>
#include "usbmonitor.h"
#include <QList>
class RAWHID_EXPORT USBSignalFilter : public QObject
{
    Q_OBJECT
private:
    QList<int> m_vid;
    int m_pid;
    int m_boardModel;
    int m_runState;
    bool portMatches(USBPortInfo port);
signals:
    void deviceDiscovered();
    void deviceRemoved();
private slots:
    void m_deviceDiscovered(USBPortInfo port);
    void m_deviceRemoved(USBPortInfo port);
public:
    USBSignalFilter(int vid, int pid, int boardModel, int runState);
    USBSignalFilter(QList<int> vid, int pid, int boardModel, int runState);
};
#endif // USBSIGNALFILTER_H
