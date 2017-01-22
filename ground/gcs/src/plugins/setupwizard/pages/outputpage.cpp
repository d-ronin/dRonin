/**
 ******************************************************************************
 *
 * @file       outputpage.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @see        The GNU Public License (GPL) Version 3
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup SetupWizard Setup Wizard
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "outputpage.h"
#include "ui_outputpage.h"
#include "setupwizard.h"

OutputPage::OutputPage(SetupWizard *wizard, QWidget *parent) :
    AbstractWizardPage(wizard, parent),

    ui(new Ui::OutputPage)
{
    ui->setupUi(this);
}

OutputPage::~OutputPage()
{
    delete ui;
}

//! Set output ranges
void OutputPage::setOutputRanges(quint16 minPulse, quint16 neutralPulse, quint16 maxPulse)
{
    QList<actuatorChannelSettings> allSettings = getWizard()->getActuatorSettings();
    for (int i = 0; i < allSettings.count(); i++) {
        actuatorChannelSettings settings = allSettings[i];
        settings.channelMin     = minPulse;
        settings.channelNeutral = neutralPulse;
        settings.channelMax     = maxPulse;
        allSettings[i] = settings;
    }
    getWizard()->setActuatorSettings(allSettings);
}

bool OutputPage::validatePage()
{
    QPointer<SetupWizard> wizard = getWizard();
    if (!wizard) {
        Q_ASSERT(false);
        qCritical() << "No wizard!";
        return false;
    }

    if (ui->oneShot125Button->isChecked()) {
        wizard->setESCType(SetupWizard::ESC_ONESHOT125);
        // This is safe to do even if they are wrong. Normal ESCS
        // ignore oneshot.
        setOutputRanges(125, 125, 250);
    } else if (ui->oneShot42Button->isChecked()) {
        wizard->setESCType(SetupWizard::ESC_ONESHOT42);
        // oneshot42 is actually oneshot125/3
        setOutputRanges(42, 42, 83);
    } else if (ui->dshot300Button->isChecked()) {
        wizard->setESCType(SetupWizard::ESC_DSHOT300);
        setOutputRanges(0, 48, 2047);
    } else if (ui->dshot600Button->isChecked()) {
        wizard->setESCType(SetupWizard::ESC_DSHOT600);
        setOutputRanges(0, 48, 2047);
    } else if (ui->dshot1200Button->isChecked()) {
        wizard->setESCType(SetupWizard::ESC_DSHOT1200);
        setOutputRanges(0, 48, 2047);
    } else {
        wizard->setESCType(SetupWizard::ESC_RAPID);
        setOutputRanges(1000, 1000, 2000);
    }

    return true;
}
