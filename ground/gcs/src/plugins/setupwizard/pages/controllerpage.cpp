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

ControllerPage::ControllerPage(SetupWizard *wizard, QWidget *parent)
    : AbstractWizardPage(wizard, parent)
    , ui(new Ui::ControllerPage)
{
    ui->setupUi(this);

    m_connectionManager = getWizard()->getConnectionManager();
    Q_ASSERT(m_connectionManager);
    connect(m_connectionManager, &Core::ConnectionManager::availableDevicesChanged, this,
            &ControllerPage::devicesChanged);

    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    Q_ASSERT(pluginManager);
    m_telemetryManager = pluginManager->getObject<TelemetryManager>();
    Q_ASSERT(m_telemetryManager);
    connect(m_telemetryManager, &TelemetryManager::connected, this,
            &ControllerPage::connectionStatusChanged);
    /* Consider us "connected" when telemetry manager says so, but
     * disconnected the instant connection manager knows the conn is down.
     */
    connect(m_connectionManager, &Core::ConnectionManager::deviceDisconnected, this,
            &ControllerPage::connectionStatusChanged);

    connect(ui->connectButton, &QAbstractButton::clicked, this, &ControllerPage::connectDisconnect);

    setupDeviceList();
}

ControllerPage::~ControllerPage()
{
    delete ui;
}

void ControllerPage::initializePage()
{
    if (anyControllerConnected()) {
        Core::IBoardType *type = getControllerType();
        setControllerType(type);
    } else {
        setControllerType(NULL);
    }
    emit completeChanged();
}

bool ControllerPage::isComplete() const
{
    Core::IBoardType *type = getControllerType();

    if (type == NULL)
        return false;

    /*
     * Only allow you to continue with setup wizard if you have a USB-type
     * connection... or if the type doesn't support USB
     * (e.g. simulation/posix)
     */
    return !type->isUSBSupported()
        || m_connectionManager->getCurrentDevice().getConName().startsWith("USB:",
                                                                           Qt::CaseInsensitive);
}

bool ControllerPage::validatePage()
{
    getWizard()->setControllerType(getControllerType());
    return true;
}

bool ControllerPage::anyControllerConnected()
{
    return m_telemetryManager->isConnected();
}

void ControllerPage::setupDeviceList()
{
    devicesChanged(m_connectionManager->getAvailableDevices());
    connectionStatusChanged();
}

void ControllerPage::setControllerType(Core::IBoardType *board)
{
    if (board == NULL)
        ui->boardTypeLabel->setText("Unknown");
    else
        ui->boardTypeLabel->setText(board->shortName());
}

void ControllerPage::devicesChanged(QLinkedList<Core::DevListItem> devices)
{
    // Get the selected item before the update if any
    QString currSelectedDeviceName = ui->deviceCombo->currentIndex() != -1
        ? ui->deviceCombo->itemData(ui->deviceCombo->currentIndex(), Qt::ToolTipRole).toString()
        : "";

    // Clear the box
    ui->deviceCombo->clear();

    int indexOfSelectedItem = -1;

    /* Sadly comparison of device doesn't seem to work... */
    QString refName = m_connectionManager->getCurrentDevice().getConName();

    // Loop and fill the combo with items from connectionmanager
    foreach (Core::DevListItem deviceItem, devices) {
        QString deviceName = deviceItem.getConName();
        if ((deviceName != refName) &&
                (!deviceName.startsWith("USB:", Qt::CaseInsensitive))) {
            continue;
        }
        ui->deviceCombo->addItem(deviceName);
        ui->deviceCombo->setItemData(ui->deviceCombo->count() - 1, deviceName, Qt::ToolTipRole);
        if (currSelectedDeviceName != "" && currSelectedDeviceName == deviceName) {
            indexOfSelectedItem = ui->deviceCombo->count() - 1;
        }
    }

    // Re select the item that was selected before if any
    if (indexOfSelectedItem != -1) {
        ui->deviceCombo->setCurrentIndex(indexOfSelectedItem);
    }
}

void ControllerPage::connectionStatusChanged()
{
    if (m_connectionManager->isConnected()) {
        ui->deviceCombo->setEnabled(false);
        ui->connectButton->setText(tr("Disconnect"));
        QString connectedDeviceName = m_connectionManager->getCurrentDevice().getConName();
        for (int i = 0; i < ui->deviceCombo->count(); ++i) {
            if (connectedDeviceName == ui->deviceCombo->itemData(i, Qt::ToolTipRole).toString()) {
                ui->deviceCombo->setCurrentIndex(i);
                break;
            }
        }

        setControllerType(getControllerType());
        qDebug() << "Connection status changed: Connected, controller type: "
                 << getControllerType();
    } else {
        ui->deviceCombo->setEnabled(true);
        ui->connectButton->setText(tr("Connect"));
        setControllerType(NULL);
        qDebug() << "Connection status changed: Disconnected";
    }
    emit completeChanged();
}

void ControllerPage::connectDisconnect()
{
    if (m_connectionManager->isConnected()) {
        m_connectionManager->disconnectDevice();
    } else {
        m_connectionManager->connectDevice(m_connectionManager->findDevice(
            ui->deviceCombo->itemData(ui->deviceCombo->currentIndex(), Qt::ToolTipRole)
                .toString()));
    }
    emit completeChanged();
}
