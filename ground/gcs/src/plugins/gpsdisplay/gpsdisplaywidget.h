/**
 ******************************************************************************
 *
 * @file       gpsdisplaywidget.h
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

#ifndef GPSDISPLAYWIDGET_H_
#define GPSDISPLAYWIDGET_H_

#include "gpsconstellationwidget.h"
#include "uavobject.h"
#include "ui_gpsdisplaywidget.h"
#include <QGraphicsView>
#include <QtSvg/QGraphicsSvgItem>
#include <QtSvg/QSvgRenderer>

class Ui_GpsDisplayWidget;

class GpsDisplayWidget : public QWidget, public Ui_GpsDisplayWidget
{
    Q_OBJECT

public:
    GpsDisplayWidget(QWidget *parent = 0);
    ~GpsDisplayWidget();

private slots:
    void setSVs(int);
    void setPosition(double, double, double);
    void setDateTime(double, double);
    void setSpeedHeading(double, double);
    void setFixType(const QString &fixtype);
    void setDOP(double hdop, double vdop, double pdop);

private:
    GpsConstellationWidget *gpsConstellation;
    QGraphicsSvgItem *marker;
};
#endif /* GPSDISPLAYWIDGET_H_ */
