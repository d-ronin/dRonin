/**
 ******************************************************************************
 *
 * @file       gpsdisplaygadget.h
 * @author     Edouard Lafargue Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup GPSGadgetPlugin GPS Gadget Plugin
 * @{
 * @brief A gadget that displays GPS status and enables basic configuration
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

#ifndef GPSDISPLAYGADGET_H_
#define GPSDISPLAYGADGET_H_

#include "gpsdisplaywidget.h"
#include "telemetryparser.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <coreplugin/iuavgadget.h>

class IUAVGadget;
class QWidget;
class QString;
class GpsDisplayWidget;

using namespace Core;

class GpsDisplayGadget : public Core::IUAVGadget
{
    Q_OBJECT
public:
    GpsDisplayGadget(QString classId, GpsDisplayWidget *widget,
                     QWidget *parent = 0);
    ~GpsDisplayGadget();

    QWidget *widget() { return m_widget; }

private:
    QPointer<GpsDisplayWidget> m_widget;
    QPointer<GPSParser> parser;
    bool connected;
    void processNewSerialData(QByteArray serialData);
};

#endif // GPSDISPLAYGADGET_H_
