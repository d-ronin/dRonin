/**
 ******************************************************************************
 * @file       configradiowidget.cpp
 * @brief      Configure the integrated radio
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2018
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

#include "configradiowidget.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/generalsettings.h>

ConfigRadioWidget::ConfigRadioWidget(QWidget *parent)
    : ConfigTaskWidget(parent)
{
    ui = new Ui::Radio();
    ui->setupUi(this);

    // Load UAVObjects to widget relations from UI file
    // using objrelation dynamic property
    autoLoadWidgets();

    // Refresh widget contents
    refreshWidgetsValues();

    // Prevent mouse wheel from changing values
    disableMouseWheelEvents();
}

ConfigRadioWidget::~ConfigRadioWidget()
{
}

void ConfigRadioWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void ConfigRadioWidget::enableControls(bool enable)
{
    Q_UNUSED(enable);
}

/**
 * @}
 * @}
 */
