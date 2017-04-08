/**
 ******************************************************************************
 *
 * @file       gpsdisplaygadget.cpp
 * @author     Edouard Lafargue Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup GPSGadgetPlugin GPS Gadget Plugin
 * @{
 * @brief A gadget that displays GPS status
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

#include "gpsdisplaygadget.h"
#include "gpsdisplaywidget.h"

GpsDisplayGadget::GpsDisplayGadget(QString classId, GpsDisplayWidget *widget, QWidget *parent)
    : IUAVGadget(classId, parent)
    , m_widget(widget)
    , connected(false)
{
    parser = new TelemetryParser();

    connect(parser, SIGNAL(sv(int)), m_widget, SLOT(setSVs(int)));
    connect(parser, SIGNAL(position(double, double, double)), m_widget,
            SLOT(setPosition(double, double, double)));
    connect(parser, SIGNAL(speedheading(double, double)), m_widget,
            SLOT(setSpeedHeading(double, double)));
    connect(parser, SIGNAL(datetime(double, double)), m_widget, SLOT(setDateTime(double, double)));
    connect(parser, SIGNAL(satellite(int, int, int, int, int)), m_widget->gpsSky,
            SLOT(updateSat(int, int, int, int, int)));
    connect(parser, SIGNAL(satellite(int, int, int, int, int)), m_widget->gpsSnrWidget,
            SLOT(updateSat(int, int, int, int, int)));
    connect(parser, SIGNAL(satellitesDone()), m_widget->gpsSnrWidget, SLOT(satellitesDone()));
    connect(parser, SIGNAL(fixtype(QString)), m_widget, SLOT(setFixType(QString)));
    connect(parser, SIGNAL(dop(double, double, double)), m_widget,
            SLOT(setDOP(double, double, double)));
}

GpsDisplayGadget::~GpsDisplayGadget()
{
    delete m_widget;
}
