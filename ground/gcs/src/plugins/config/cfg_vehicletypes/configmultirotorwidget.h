/**
 ******************************************************************************
 *
 * @file       configairframetwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Airframe configuration panel
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
#ifndef CONFIGMULTIROTORWIDGET_H
#define CONFIGMULTIROTORWIDGET_H

#include "ui_airframe.h"
#include "uavobjectwidgetutils/configtaskwidget.h"
#include "cfg_vehicletypes/vehicleconfig.h"

#include "extensionsystem/pluginmanager.h"
#include "uavobjects/uavobjectmanager.h"
#include "uavobjects/uavobject.h"
#include "uavtalk/telemetrymanager.h"
#include <QWidget>
#include <QList>
#include <QItemDelegate>

class Ui_Widget;

class ConfigMultiRotorWidget : public VehicleConfig
{
    Q_OBJECT

public:
    ConfigMultiRotorWidget(Ui_AircraftWidget *aircraft = nullptr, QWidget *parent = nullptr);
    ~ConfigMultiRotorWidget();

    friend class ConfigVehicleTypeWidget;

private:
    Ui_AircraftWidget *m_aircraft;

    QWidget *uiowner;
    QGraphicsSvgItem *quad;

    bool setupQuad(bool pLayout);
    bool setupHexa(bool pLayout);
    bool setupOcto();
    bool setupMultiRotorMixer(double mixerFactors[8][3]);
    void setupMotors(QList<QString> motorList, SystemSettings::AirframeTypeOptions vehicle);
    void setupQuadMotor(int channel, double roll, double pitch, double yaw);

    float invertMotors;

    virtual void ResetActuators(GUIConfigDataUnion *configData);
    static QStringList getChannelDescriptions();
    static const QString CHANNELBOXNAME;
    static const QString CHANNELLABELNAME;
    void setYawMixLevel(int);

    void drawAirframe(SystemSettings::AirframeTypeOptions multiRotorType);

private slots:
    virtual void setupUI(SystemSettings::AirframeTypeOptions airframeType);
    void refreshAirframeWidgetsValues(SystemSettings::AirframeTypeOptions frameType);
    virtual SystemSettings::AirframeTypeOptions updateConfigObjectsFromWidgets();
    virtual bool throwConfigError(int numMotors);

protected:
signals:
    void configurationChanged();
};

#endif // CONFIGMULTIROTORWIDGET_H
