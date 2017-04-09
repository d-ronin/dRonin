/**
 ******************************************************************************
 *
 * @file       brainconfiguration.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_BrainFPV BrainFPV boards support Plugin
 * @{
 * @brief Plugin to support boards by BrainFPV LLC
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

#include "smartsavebutton.h"
#include "brainconfiguration.h"
#include "ui_brainconfiguration.h"

#include "hwbrain.h"

BrainConfiguration::BrainConfiguration(QWidget *parent)
    : ConfigTaskWidget(parent)
    , ui(new Ui::BrainConfiguration)
{
    ui->setupUi(this);

    addApplySaveButtons(ui->applySettings, ui->saveSettings);
    addUAVObjectToWidgetRelation("HwBrain", "RxPort", ui->cmbRxPort);
    addUAVObjectToWidgetRelation("HwBrain", "RxPortUsart", ui->cmbRxPortUsart);
    addUAVObjectToWidgetRelation("HwBrain", "MainPort", ui->cmbMainPort);
    addUAVObjectToWidgetRelation("HwBrain", "FlxPort", ui->cmbFlxPort);

    addUAVObjectToWidgetRelation("HwBrain", "Magnetometer", ui->cmbMagnetometer);
    addUAVObjectToWidgetRelation("HwBrain", "ExtMagOrientation", ui->cmbExtMagOrientation);
    addUAVObjectToWidgetRelation("HwBrain", "USB_HIDPort", ui->cmbUsbHidPort);
    addUAVObjectToWidgetRelation("HwBrain", "USB_VCPPort", ui->cmbUsbVcpPort);
    addUAVObjectToWidgetRelation("HwBrain", "GyroFullScale", ui->cmbGyroRange);
    addUAVObjectToWidgetRelation("HwBrain", "AccelFullScale", ui->cmbAccelRange);
    addUAVObjectToWidgetRelation("HwBrain", "MPU9250GyroLPF", ui->cmbGyroLpf);
    addUAVObjectToWidgetRelation("HwBrain", "MPU9250AccelLPF", ui->cmbAccelLpf);
    addUAVObjectToWidgetRelation("HwBrain", "DSMxMode", ui->cbDsmxMode);

    // Load UAVObjects to widget relations from UI file
    // using objrelation dynamic property
    autoLoadWidgets();

    // enableControls(false);
    enableControls(true);
    populateWidgets();
    refreshWidgetsValues();
    forceConnectedState();

    img = QPixmap(":/brainfpv/images/brain.png");
    ui->imgLabel->setPixmap(img);
}

BrainConfiguration::~BrainConfiguration()
{
    delete ui;
}

void BrainConfiguration::openHelp()
{
    QDesktopServices::openUrl(QUrl("http://www.brainfpv.com/support", QUrl::StrictMode));
}
