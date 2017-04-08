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
 * with this program; if not, see <http://www.gnu.org/licenses/>
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

    addUAVObjectToWidgetRelation("HwBrainRE1", "MultiPortMode",ui->cmbMultiPortMode);
    addUAVObjectToWidgetRelation("HwBrainRE1", "MultiPortSerial",ui->cmbMultiPortSerial);
    addUAVObjectToWidgetRelation("HwBrainRE1", "MultiPortSerial2",ui->cmbMultiPortSerial2);

    addUAVObjectToWidgetRelation("HwBrainRE1", "I2CExtBaro",ui->cmbI2CExtBaro);
    addUAVObjectToWidgetRelation("HwBrainRE1", "I2CExtMag",ui->cmbI2CExtMag);
    addUAVObjectToWidgetRelation("HwBrainRE1", "ExtMagOrientation",ui->cmbExtMagOrientation);

    addUAVObjectToWidgetRelation("HwBrainRE1", "USB_HIDPort", ui->cmbUsbHidPort);
    addUAVObjectToWidgetRelation("HwBrainRE1", "USB_VCPPort", ui->cmbUsbVcpPort);

    addUAVObjectToWidgetRelation("HwBrainRE1", "IRProtocol", ui->cmbIRProtocol);
    addUAVObjectToWidgetRelation("HwBrainRE1", "IRIDILap", ui->sbILapID);
    addUAVObjectToWidgetRelation("HwBrainRE1", "IRIDTrackmate", ui->sbTrackmateID);
    connect(ui->pbGenerateILap ,&QAbstractButton::clicked, 
        this, &BrainRE1Configuration::generateILapID);
    connect(ui->pbGenerateTrackmate ,&QAbstractButton::clicked,
        this, &BrainRE1Configuration::generateTrackmateID);

    addUAVObjectToWidgetRelation("HwBrainRE1", "BuzzerType", ui->cmbBuzzerType);
    addUAVObjectToWidgetRelation("HwBrainRE1", "VideoSyncDetectorThreshold", ui->sbVideoSyncDetectorThreshold);

    // Load UAVObjects to widget relations from UI file
    // using objrelation dynamic property
    autoLoadWidgets();

    enableControls(true);
    populateWidgets();
    refreshWidgetsValues();
    forceConnectedState();

    QPixmap img;
    img = QPixmap(":/brainfpv/images/brainre1.png");
    ui->imgLabel->setPixmap(img);

    connect(ui->cmbMultiPortMode, SIGNAL(currentIndexChanged(int)), this, SLOT(mpChanged(int)));
    connect(ui->cmbI2CExtMag, SIGNAL(currentIndexChanged(int)), this, SLOT(extMagChanged(int)));

    mpChanged(re1_settings_obj->getMultiPortMode());
    extMagChanged(re1_settings_obj->getI2CExtMag());
}

BrainRE1Configuration::~BrainRE1Configuration()
{
    delete ui;
}

void BrainRE1Configuration::widgetsContentsChanged()
{
    ConfigTaskWidget::widgetsContentsChanged();

    if ((ui->cmbUsbHidPort->currentIndex() == HwBrainRE1::USB_HIDPORT_USBTELEMETRY) && (ui->cmbUsbVcpPort->currentIndex() == HwBrainRE1::USB_VCPPORT_USBTELEMETRY))
    {
        enableControls(false);
        ui->lblMsg->setText(tr("Warning: you have configured both USB HID Port and USB VCP Port for telemetry, this currently is not supported"));
    }
    else if ((ui->cmbUsbHidPort->currentIndex() != HwBrainRE1::USB_HIDPORT_USBTELEMETRY) && (ui->cmbUsbVcpPort->currentIndex() != HwBrainRE1::USB_VCPPORT_USBTELEMETRY))
    {
        enableControls(false);
        ui->lblMsg->setText(tr("Warning: you have disabled USB Telemetry on both USB HID Port and USB VCP Port, this currently is not supported"));
    }
    else
    {
        ui->lblMsg->setText("");
        enableControls(true);
    }
}

void BrainRE1Configuration::openHelp()
{
    QDesktopServices::openUrl( QUrl("http://www.brainfpv.com/support", QUrl::StrictMode) );
}

void BrainRE1Configuration::generateILapID()
{
    ui->sbILapID->setValue(generateRandomNumber(9999999));
}

void BrainRE1Configuration::generateTrackmateID()
{
    uint16_t trackmate_id;
    bool valid = false;
    while (!valid) {
        trackmate_id = generateRandomNumber(0xFFF);
        // It seems like the ID has some weird requirements:
        // 1st nibble is 0, 2nd nibble is not 0, 1, 8, or F
        // 3rd and 4th nibbles are not 0 or F
        valid = ((trackmate_id & 0x0F00) >> 8) != 0x0;
        valid = valid && (((trackmate_id & 0x0F00) >> 8) != 0x1);
        valid = valid && (((trackmate_id & 0x0F00) >> 8) != 0x8);
        valid = valid && (((trackmate_id & 0x0F00) >> 8) != 0xF);
        valid = valid && (((trackmate_id & 0x00F0) >> 4) != 0x0);
        valid = valid && (((trackmate_id & 0x00F0) >> 4) != 0xF);
        valid = valid && ((trackmate_id & 0x000F) != 0x0);
        valid = valid && ((trackmate_id & 0x000F) != 0xF);
    }
    ui->sbTrackmateID->setValue(trackmate_id);
}

int BrainRE1Configuration::generateRandomNumber(int max)
{
    //std::default_random_engine generator;
    std::random_device generator;
    std::uniform_int_distribution<int> distribution(0, max);
    //generator.seed(QTime::currentTime().msec());
    return distribution(generator);
}

void BrainRE1Configuration::mpChanged(int idx)
{
    QPixmap img;
    if (idx == HwBrainRE1::MULTIPORTMODE_NORMAL) {
        img = QPixmap(":/brainfpv/images/re1_mp_normal.png");
        ui->cmbMultiPortSerial2->setHidden(true);
        ui->labelMultiPortSerial2->setHidden(true);
    }
    else {
        img = QPixmap(":/brainfpv/images/re1_mp_4pwm.png");
        ui->cmbMultiPortSerial2->setHidden(false);
        ui->labelMultiPortSerial2->setHidden(false);
    }
    ui->imgLabelMp->setPixmap(img);
}

void BrainRE1Configuration::extMagChanged(int idx)
{
    if (idx == 0) {
        ui->cmbExtMagOrientation->setHidden(true);
        ui->labelExtMagOrientation->setHidden(true);
    }
    else {
        ui->cmbExtMagOrientation->setHidden(false);
        ui->labelExtMagOrientation->setHidden(false);
    }
}
