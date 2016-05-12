/**
 ******************************************************************************
 *
 * @file       BrainRE1Configuration.cpp
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "smartsavebutton.h"
#include "brainre1configuration.h"
#include "ui_brainre1configuration.h"

#include "hwbrainre1.h"

#include <random>


BrainRE1Configuration::BrainRE1Configuration(QWidget *parent) :
    ConfigTaskWidget(parent),
    ui(new Ui::BrainRE1Configuration)
{
    ui->setupUi(this);

    re1_settings_obj = HwBrainRE1::GetInstance(getObjectManager());

    addApplySaveButtons(ui->applySettings,ui->saveSettings);
    addUAVObjectToWidgetRelation("HwBrainRE1", "RxPort",ui->cmbRxPort);
    addUAVObjectToWidgetRelation("HwBrainRE1", "DSMxMode", ui->cmbDSMxMode);

    addUAVObjectToWidgetRelation("HwBrainRE1", "SerialPort",ui->cmbSerialPort);
    addUAVObjectToWidgetRelation("HwBrainRE1", "MultiPort",ui->cmbMultiPort);

    addUAVObjectToWidgetRelation("HwBrainRE1", "USB_HIDPort", ui->cmbUsbHidPort);
    addUAVObjectToWidgetRelation("HwBrainRE1", "USB_VCPPort", ui->cmbUsbVcpPort);

    addUAVObjectToWidgetRelation("HwBrainRE1", "IRProtocol", ui->cmbIRProtocol);
    addUAVObjectToWidgetRelation("HwBrainRE1", "IRIDILap", ui->sbILapID);
    addUAVObjectToWidgetRelation("HwBrainRE1", "IRIDTrackmate", ui->sbTrackmateID);
    connect(ui->pbGenerateILap ,SIGNAL(clicked()) ,this, SLOT(generateILapID()));
    connect(ui->pbGenerateTrackmate ,SIGNAL(clicked()) ,this, SLOT(generateTrackmateID()));

    addUAVObjectToWidgetRelation("HwBrainRE1", "NumberOfLEDs", ui->sbNumberOfLEDs);
    addUAVObjectToWidgetRelation("HwBrainRE1", "LEDColor", ui->cmbLEDColor);

    connect(re1_settings_obj, SIGNAL(CustomLEDColor_0Changed(quint8)), this, SLOT(getCustomLedColor()));
    connect(re1_settings_obj, SIGNAL(CustomLEDColor_1Changed(quint8)), this, SLOT(getCustomLedColor()));
    connect(re1_settings_obj, SIGNAL(CustomLEDColor_2Changed(quint8)), this, SLOT(getCustomLedColor()));
    connect(ui->clrbCustomLEDColor, SIGNAL(colorChanged(const QColor)), this, SLOT(setCustomLedColor(const QColor)));

    addUAVObjectToWidgetRelation("HwBrainRE1", "FailsafeBuzzer", ui->cmbFailsafeBuzzer);

    // Load UAVObjects to widget relations from UI file
    // using objrelation dynamic property
    autoLoadWidgets();

    connect(ui->help,SIGNAL(clicked()), this, SLOT(openHelp()));
    enableControls(true);
    populateWidgets();
    refreshWidgetsValues();
    forceConnectedState();
    
    img = QPixmap(":/brainfpv/images/brainre1.png");
    ui->imgLabel->setPixmap(img);
}

BrainRE1Configuration::~BrainRE1Configuration()
{
    delete ui;
}

void BrainRE1Configuration::openHelp()
{
    QDesktopServices::openUrl( QUrl("http://wwww.brainfpv.com/support", QUrl::StrictMode) );
}

void BrainRE1Configuration::generateILapID()
{
    ui->sbILapID->setValue(generateRandomNumber(9999999));
}

void BrainRE1Configuration::generateTrackmateID()
{
    ui->sbTrackmateID->setValue(generateRandomNumber(9999));
}

int BrainRE1Configuration::generateRandomNumber(int max)
{
    //std::default_random_engine generator;
    std::random_device generator;
    std::uniform_int_distribution<int> distribution(0, max);
    //generator.seed(QTime::currentTime().msec());
    return distribution(generator);
}

void BrainRE1Configuration::getCustomLedColor()
{
    QColor color = QColor::fromRgb(re1_settings_obj->getCustomLEDColor(0),
                                   re1_settings_obj->getCustomLEDColor(1),
                                   re1_settings_obj->getCustomLEDColor(2));
    ui->clrbCustomLEDColor->setColor(color);
}

void BrainRE1Configuration::setCustomLedColor(const QColor color)
{
    re1_settings_obj->setCustomLEDColor(0, color.red());
    re1_settings_obj->setCustomLEDColor(1, color.green());
    re1_settings_obj->setCustomLEDColor(2, color.blue());
}

