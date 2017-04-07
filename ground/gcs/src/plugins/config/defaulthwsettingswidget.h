/**
 ******************************************************************************
 *
 * @file       defaultccattitudewidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Placeholder for attitude settings widget until board connected.
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
#ifndef DEFAULTHWSETTINGSt_H
#define DEFAULTHWSETTINGSt_H

#include "../uavobjectwidgetutils/configtaskwidget.h"
#include "uavobject.h"
#include "ui_defaulthwsettings.h"
#include <QWidget>

class Ui_Widget;

class DefaultHwSettingsWidget : public ConfigTaskWidget
{
    Q_OBJECT

public:
    explicit DefaultHwSettingsWidget(UAVObject *settingsObj,
                                     QWidget *parent = 0);
    ~DefaultHwSettingsWidget();

private:
    Ui_DefaultHwSettings *defaultHWSettingsWidget;
};

#endif // DEFAULTHWSETTINGSt_H
