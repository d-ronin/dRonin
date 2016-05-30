/**
 ******************************************************************************
 * @file       dtfcconfiguration.cpp
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_DTF DTF boards support Plugin
 * @{
 * @brief Plugin to support DTF boards
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

#include "smartsavebutton.h"
#include "dtfcconfiguration.h"
#include "ui_dtfcconfiguration.h"

#include "hwdtfc.h"

DTFcConfiguration::DTFcConfiguration(QWidget *parent) :
    ConfigTaskWidget(parent),
    ui(new Ui::DTFcConfiguration)
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

DTFcConfiguration::~DTFcConfiguration()
{
    delete ui;
}
