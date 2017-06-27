/**
 ******************************************************************************
 * @file       luxconfiguration.cpp
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_Brotronics Brotronics boards support Plugin
 * @{
 * @brief Plugin to support Brotronics boards
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

#include "uavobjectwidgetutils/smartsavebutton.h"
#include "luxconfiguration.h"
#include "ui_luxconfiguration.h"

#include "hwlux.h"

LuxConfiguration::LuxConfiguration(QWidget *parent)
    : ConfigTaskWidget(parent)
    , ui(new Ui::LuxConfiguration)
{
    ui->setupUi(this);

    // Load UAVObjects to widget relations from UI file
    // using objrelation dynamic property
    autoLoadWidgets();

    enableControls(true);
    populateWidgets();
    refreshWidgetsValues();
    forceConnectedState();
}

LuxConfiguration::~LuxConfiguration()
{
    delete ui;
}
