/**
 ******************************************************************************
 *
 * @file       boardtype_unknown.cpp
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

#include "boardtype_unknown.h"
#include "ui_boardtype_unknown.h"
#include "setupwizard.h"

BoardtypeUnknown::BoardtypeUnknown(SetupWizard *wizard, QWidget *parent)
    : AbstractWizardPage(wizard, parent)
    ,

    ui(new Ui::BoardtypeUnknown)
{
    ui->setupUi(this);
}

BoardtypeUnknown::~BoardtypeUnknown()
{
    delete ui;
}

bool BoardtypeUnknown::validatePage()
{
    return true;
}

void BoardtypeUnknown::setFailureType(FailureType type)
{
    switch (type) {
    case UNKNOWN_FIRMWARE:
        ui->lblReason->setText(
            tr("The firmware version on the board does not match this version of GCS."));
        break;
    case UNKNOWN_BOARD:
        ui->lblReason->setText(tr("Unknown board type."));
        break;
    default:
        break;
    }
}
