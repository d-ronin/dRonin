/**
 ******************************************************************************
 *
 * @file       inputpage.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @see        The GNU Public License (GPL) Version 3
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup SetupWizard Setup Wizard
 * @{
 * @brief
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

#include "inputpage.h"
#include "ui_inputpage.h"
#include "setupwizard.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"

InputPage::InputPage(SetupWizard *wizard, QWidget *parent) :
    AbstractWizardPage(wizard, parent),

    ui(new Ui::InputPage)
{
    ui->setupUi(this);
    
    // disable invalid input types
    Core::IBoardType* board = getControllerType();
    if (board) {
        ui->pwmButton->setEnabled(board->isInputConfigurationSupported(Core::IBoardType::INPUT_TYPE_PWM));
        ui->ppmButton->setEnabled(board->isInputConfigurationSupported(Core::IBoardType::INPUT_TYPE_PPM));
        ui->hottSumDButton->setEnabled(board->isInputConfigurationSupported(Core::IBoardType::INPUT_TYPE_HOTTSUMD));
        ui->hottSumHButton->setEnabled(board->isInputConfigurationSupported(Core::IBoardType::INPUT_TYPE_HOTTSUMH));
        ui->sbusButton->setEnabled(board->isInputConfigurationSupported(Core::IBoardType::INPUT_TYPE_SBUS));
        ui->sbusNonInvertedButton->setEnabled(board->isInputConfigurationSupported(Core::IBoardType::INPUT_TYPE_SBUSNONINVERTED));
        ui->spectrumButton->setEnabled(board->isInputConfigurationSupported(Core::IBoardType::INPUT_TYPE_DSM));
        ui->ibusButton->setEnabled(board->isInputConfigurationSupported(Core::IBoardType::INPUT_TYPE_IBUS));
        ui->srxlButton->setEnabled(board->isInputConfigurationSupported(Core::IBoardType::INPUT_TYPE_SRXL));
        ui->tbsCrossfireButton->setEnabled(board->isInputConfigurationSupported(Core::IBoardType::INPUT_TYPE_TBSCROSSFIRE));
    }
    // the default might have been disabled, choose one that's available
    foreach (QToolButton *button, findChildren<QToolButton *>()) {
        if (button->isEnabled()) {
            button->setChecked(true);
            break;
        }
    }
}

InputPage::~InputPage()
{
    delete ui;
}

bool InputPage::validatePage()
{
    if (ui->pwmButton->isChecked()) {
        getWizard()->setInputType(Core::IBoardType::INPUT_TYPE_PWM);
    } else if (ui->ppmButton->isChecked()) {
        getWizard()->setInputType(Core::IBoardType::INPUT_TYPE_PPM);
    } else if (ui->sbusButton->isChecked()) {
        getWizard()->setInputType(Core::IBoardType::INPUT_TYPE_SBUS);
    } else if (ui->sbusNonInvertedButton->isChecked()) {
        getWizard()->setInputType(Core::IBoardType::INPUT_TYPE_SBUSNONINVERTED);
    } else if (ui->spectrumButton->isChecked()) {
        getWizard()->setInputType(Core::IBoardType::INPUT_TYPE_DSM);
    } else if (ui->hottSumDButton->isChecked()) {
        getWizard()->setInputType(Core::IBoardType::INPUT_TYPE_HOTTSUMD);
    } else if (ui->hottSumHButton->isChecked()) {
        getWizard()->setInputType(Core::IBoardType::INPUT_TYPE_HOTTSUMH);
    } else if (ui->ibusButton->isChecked()) {
        getWizard()->setInputType(Core::IBoardType::INPUT_TYPE_IBUS);
    } else if (ui->srxlButton->isChecked()) {
        getWizard()->setInputType(Core::IBoardType::INPUT_TYPE_SRXL);
    } else if (ui->tbsCrossfireButton->isChecked()) {
        getWizard()->setInputType(Core::IBoardType::INPUT_TYPE_TBSCROSSFIRE);
    } else {
        getWizard()->setInputType(Core::IBoardType::INPUT_TYPE_PWM);
    }
    getWizard()->setRestartNeeded(getWizard()->isRestartNeeded() || restartNeeded(getWizard()->getInputType()));

    return true;
}

/**
 * @brief InputPage::restartNeeded Check if the requested input type is currently
 * selected
 * @param selectedType the requested input type
 * @return true if changing input type and should restart, false otherwise
 */
bool InputPage::restartNeeded(Core::IBoardType::InputType selectedType)
{
    Core::IBoardType* board = getWizard()->getControllerType();
    Q_ASSERT(board);
    if (!board)
        return true;

    // Map from the enums used in SetupWizard to IBoardType
    Core::IBoardType::InputType boardInputType = board->getInputType();
    return (selectedType != boardInputType);
}
