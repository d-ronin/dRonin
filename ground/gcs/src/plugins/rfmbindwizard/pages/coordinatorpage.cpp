/**
 ******************************************************************************
 *
 * @file       coordinatorpage.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @see        The GNU Public License (GPL) Version 3
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup RfmBindWizard Setup Wizard
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

#include "coordinatorpage.h"
#include "ui_coordinatorpage.h"
#include "rfmbindwizard.h"

#include <extensionsystem/pluginmanager.h>
#include <uavobjectutil/uavobjectutilmanager.h>

#include <QTimer>
#include <coreplugin/iboardtype.h>

CoordinatorPage::CoordinatorPage(RfmBindWizard *wizard, QWidget *parent) :
    RadioProbePage(wizard, parent), ui(new Ui::CoordinatorPage),
    m_coordinatorConfigured(false)
{
    qDebug() << "CoordinatorPage constructor";
    ui->setupUi(this);
    ui->setCoordinator->setEnabled(false);

    connect(ui->setCoordinator, &QAbstractButton::clicked, this, &CoordinatorPage::configureCoordinator);    
    connect(this, &RadioProbePage::probeChanged, this, &CoordinatorPage::updateProbe);
}

CoordinatorPage::~CoordinatorPage()
{
    delete ui;
}

void CoordinatorPage::initializePage()
{
    emit completeChanged();
}

bool CoordinatorPage::isComplete() const
{
    return m_coordinatorConfigured;
}

bool CoordinatorPage::validatePage()
{
    disconnect(this);
    return true;
}

//! Update the UI when the probe status changes
void CoordinatorPage::updateProbe(bool probed)
{
    Core::IBoardType *board = getBoardType();
    if (probed && board) {
        ui->boardTypeLabel->setText(board->shortName());

        // Do not allow performing this for multiple boards
        if (!m_coordinatorConfigured)
            ui->setCoordinator->setEnabled(true);
    } else {
        ui->boardTypeLabel->setText("Unknown");
        ui->setCoordinator->setEnabled(false);
    }
}

bool CoordinatorPage::configureCoordinator()
{
    Core::IBoardType *board = getBoardType();
    if (board == NULL)
        return false;

    // Get the ID for this board and make it a coordinator
    quint32 rfmId = board->getRfmID();

    // Store the coordinator ID
    getWizard()->setCoordID(rfmId);

    board->bindRadio(0, getWizard()->getMaxBps(), getWizard()->getMaxRfPower(),
                     getWizard()->getLinkMode(),getWizard()->getMinChannel(), getWizard()->getMaxChannel());

    m_coordinatorConfigured = true;
    ui->setCoordinator->setEnabled(false);

    qDebug() << "Coordinator ID: " << rfmId;

    emit completeChanged();
    stopProbing();

    return true;
}
