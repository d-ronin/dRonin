/**
 ******************************************************************************
 *
 * @file       configstabilizationwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Stabilization configuration panel
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
#ifndef CONFIGSTABILIZATIONWIDGET_H
#define CONFIGSTABILIZATIONWIDGET_H

#include "ui_stabilization.h"
#include "../uavobjectwidgetutils/configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include "stabilizationsettings.h"
#include <QWidget>
#include <QTimer>
#include <expocurve.h>


class ConfigStabilizationWidget: public ConfigTaskWidget
{
    Q_OBJECT

public:
    ConfigStabilizationWidget(QWidget *parent = 0);
    ~ConfigStabilizationWidget();

private:
    Ui_StabilizationWidget *m_stabilization;

    UAVObject *manualControlSettings;

    bool updateInProgress;

    void updateGraphs();
    void setDerivedControlsEnabled(bool enable);

private slots:
    void linkCheckBoxes(int value);
    void ratesLink(int value);

    void processLinkedWidgets(QWidget*);
    void setMaximums();
    void derivedValuesChanged();
    void sourceValuesChanged();

    void applyRateLimits();

    void hangtimeDurationChanged();
    void hangtimeToggle(bool enabled);
    void enableDerivedControls();
    void disableDerivedControls();
};

#endif // ConfigStabilizationWidget_H
