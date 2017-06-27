/**
 ******************************************************************************
 *
 * @file       configcamerastabilizationtwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Telemetry configuration panel
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
#ifndef CONFIGCAMERASTABILIZATIONWIDGET_H
#define CONFIGCAMERASTABILIZATIONWIDGET_H

#include "ui_camerastabilization.h"
#include "../uavobjectwidgetutils/configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjects/uavobjectmanager.h"
#include "uavobjects/uavobject.h"
#include "camerastabsettings.h"
#include <QWidget>
#include <QList>

class ConfigCameraStabilizationWidget : public ConfigTaskWidget
{
    Q_OBJECT

public:
    ConfigCameraStabilizationWidget(QWidget *parent = 0);
    ~ConfigCameraStabilizationWidget();

private:
    Ui_CameraStabilizationWidget *m_camerastabilization;
    void refreshWidgetsValues(UAVObject *obj);
    void updateObjectsFromWidgets();

private slots:
    void defaultRequestedSlot(int group);
};

#endif // CONFIGCAMERASTABILIZATIONWIDGET_H
