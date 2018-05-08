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

#include "openlrs.h"

#include "ui_integratedradio.h"

ConfigRadioWidget::ConfigRadioWidget(QWidget *parent)
    : ConfigTaskWidget(parent)
{
    ui = new Ui::radio();
    ui->setupUi(this);

    // Load UAVObjects to widget relations from UI file
    // using objrelation dynamic property
    autoLoadWidgets();

    // Refresh widget contents
    ConfigTaskWidget::refreshWidgetsValues();

    // Prevent mouse wheel from changing values
    disableMouseWheelEvents();

    connect(ui->rb_tx, &QRadioButton::toggled, this,
            &ConfigRadioWidget::roleChanged);
    connect(ui->rb_rx, &QRadioButton::toggled, this,
            &ConfigRadioWidget::roleChanged);
    connect(ui->rb_disabled, &QRadioButton::toggled, this,
            &ConfigRadioWidget::roleChanged);
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

void ConfigRadioWidget::roleChanged(bool ignored)
{
    (void) ignored;

    if (ui->rb_tx->isChecked()) {
        ui->groupTxSettings->setEnabled(true);
        ui->groupRxSettings->setEnabled(false);
    } else if (ui->rb_rx->isChecked()) {
        ui->groupTxSettings->setEnabled(false);
        ui->groupRxSettings->setEnabled(true);
    } else {
        ui->groupTxSettings->setEnabled(false);
        ui->groupRxSettings->setEnabled(false);
    }
}

void ConfigRadioWidget::refreshWidgetsValues(UAVObject *obj)
{
    (void) obj;

    OpenLRS *olrsObj = OpenLRS::GetInstance(getObjectManager());

    OpenLRS::DataFields openlrs = olrsObj->getData();

    switch (openlrs.role) {
    case OpenLRS::ROLE_DISABLED:
    default:
        ui->rb_disabled->setChecked(true);
        break;
    case OpenLRS::ROLE_RX:
        ui->rb_rx->setChecked(true);
        break;
    case OpenLRS::ROLE_TX:
        ui->rb_tx->setChecked(true);
        break;
    }

    ConfigTaskWidget::refreshWidgetsValues();
}

void ConfigRadioWidget::updateObjectsFromWidgets()
{
    OpenLRS *olrsObj = OpenLRS::GetInstance(getObjectManager());

    if (ui->rb_tx->isChecked()) {
        olrsObj->setrole(OpenLRS::ROLE_TX);
    } else if (ui->rb_rx->isChecked()) {
        olrsObj->setrole(OpenLRS::ROLE_RX);
    } else {
        olrsObj->setrole(OpenLRS::ROLE_DISABLED);
    }

    ConfigTaskWidget::updateObjectsFromWidgets();
}

/**
 * @}
 * @}
 */
