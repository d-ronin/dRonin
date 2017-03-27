/**
 ******************************************************************************
 *
 * @file       controllerpage.cpp
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

#include "controllerpage.h"
#include "ui_controllerpage.h"
#include "setupwizard.h"

#include <extensionsystem/pluginmanager.h>
#include <uavobjectutil/uavobjectutilmanager.h>

ControllerPage::ControllerPage(SetupWizard *wizard, QWidget *parent) :
    AbstractWizardPage(wizard, parent),
    ui(new Ui::ControllerPage)
{
    ui->setupUi(this);

    m_connectionManager = getWizard()->getConnectionManager();
    Q_ASSERT(m_connectionManager);

    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    Q_ASSERT(pluginManager);
    m_telemtryManager = pluginManager->getObject<TelemetryManager>();
    Q_ASSERT(m_telemtryManager);

    connect(m_connectionManager, &Core::ConnectionManager::availableDevicesChanged,
            this, &ControllerPage::devicesChanged);

    connect(m_connectionManager, &Core::ConnectionManager::connectionStatusChanged,
            this, &ControllerPage::connectionStatusChanged);

    connect(ui->connectButton, &QPushButton::clicked, this, &ControllerPage::connectDisconnect);

    setupDeviceList();
}

ControllerPage::~ControllerPage()
{
    delete ui;
}

void ControllerPage::initializePage()
{
    if (anyControllerConnected()) {
        Core::IBoardType* type = getControllerType();
        setControllerType(type);
    } else {
        setControllerType(NULL);
    }
    emit completeChanged();
}

bool ControllerPage::isComplete() const
{
    Core::IBoardType* type = getControllerType();

    if (type == NULL)
        return false;

    return !type->isUSBSupported() ||
           m_connectionManager->getCurrentDevice().getConName().startsWith("USB:", Qt::CaseInsensitive);
}

bool ControllerPage::validatePage()
{
    getWizard()->setControllerType(getControllerType());
    return true;
}

bool ControllerPage::anyControllerConnected()
{
    return m_telemtryManager->isConnected();
}

void ControllerPage::setupDeviceList()
{
    devicesChanged(m_connectionManager->getAvailableDevices());
    connectionStatusChanged(m_connectionManager->isConnected());
}

void ControllerPage::setControllerType(Core::IBoardType *board)
{
    if (!board)
        ui->boardTypeLabel->setText("Unknown");
    else
        ui->boardTypeLabel->setText(board->shortName());
}

/**
 * @brief ControllerPage::devicesChanged
 * @param devices
 * @todo This would be simplified if connection manager told us which device was added/removed
 */
void ControllerPage::devicesChanged(QLinkedList<Core::DevListItem> devices)
{
    // Get the selected item so it can be selected again at the end
    const QString selectedDeviceName = ui->deviceCombo->currentData(Qt::ToolTipRole).toString();

    ui->deviceCombo->clear();
    // Loop and fill the combo with items from connectionmanager (too dumb to just add/remove one device)
    for (const auto &deviceItem : devices) {
        const QString deviceName = deviceItem.getConName();

        /**
         * @todo Allow us to use sim for testing setup wizard
         */
        // no point adding devices which can't be connected
        // we would need to handle that by disabling connect button etc.
        if (!deviceName.startsWith("USB:", Qt::CaseInsensitive))
            continue;

        ui->deviceCombo->addItem(deviceName);
        if (selectedDeviceName == deviceName)
            ui->deviceCombo->setCurrentIndex(ui->deviceCombo->count() - 1);
    }

    // This signal arrives after connectionStatusChanged so we need
    // to make sure we select the device if one is connected
    if (m_connectionManager->isConnected())
        ui->deviceCombo->setCurrentText(m_connectionManager->getCurrentDevice().getConName());
}

void ControllerPage::connectionStatusChanged(bool connected)
{
    ui->deviceCombo->setEnabled(!connected);
    ui->connectButton->setText(connected ? tr("Disconnect") : tr("Connect"));
    setControllerType(connected ? getControllerType() : Q_NULLPTR);

    emit completeChanged();
}

void ControllerPage::connectDisconnect()
{
    if (m_connectionManager->isConnected()) {
        m_connectionManager->disconnectDevice();
    } else {
        m_connectionManager->connectDevice(m_connectionManager->findDevice(ui->deviceCombo->currentText()));
    }
    emit completeChanged();
}
