/**
 ******************************************************************************
 * @file       configmodulewidget.h
 * @brief      Configure the optional modules
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
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
#ifndef CONFIGMODULEWIDGET_H
#define CONFIGMODULEWIDGET_H

#include "ui_modules.h"

#include "uavobjectwidgetutils/configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"

namespace Ui {
    class Modules;
}

class ConfigModuleWidget: public ConfigTaskWidget
{
    Q_OBJECT

public:
        ConfigModuleWidget(QWidget *parent = 0);
        ~ConfigModuleWidget();

private slots:
    void updateAirspeedGroupbox(UAVObject *);
    void enableAirspeedTypeGPS(bool);
    void enableAirspeedTypePitot(bool);
    void toggleBatteryMonitoringPin();
    void toggleBatteryMonitoringGb();
    void updateVoltageRatio();
    void updateVoltageFactor();
    void updateVoltageFactorFromUavo(float value);

    void recheckTabs();
    void objectUpdated(UAVObject * obj, bool success);
    void autoCellDetectionToggled(bool checked);
    void maxCellVoltageChanged(double value);
    void ledTabUpdate(UAVObject *obj);
    void ledTabSetColor();

private:
    void setupLedTab();
    /* To activate the appropriate tabs */
    void enableBatteryTab(bool enabled);
    void enableAirspeedTab(bool enabled);
    void enableVibrationTab(bool enabled);
    void enableHoTTTelemetryTab(bool enabled);
    void enableGeofenceTab(bool enabled);
    void enableGpsTab(bool enabled);
    void enableLoggingTab(bool enabled);

    void refreshAdcNames(void);

    Ui::Modules *ui;

protected:
    void resizeEvent(QResizeEvent *event);
    virtual void enableControls(bool enable);
};

#endif // CONFIGMODULEWIDGET_H
