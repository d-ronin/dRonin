/**
 ******************************************************************************
 *
 * @file       telemetrymanager.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVTalkPlugin UAVTalk Plugin
 * @{
 * @brief The UAVTalk protocol plugin
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

#include "telemetrymanager.h"
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>

TelemetryManager::TelemetryManager()
    : m_connected(false)
{
    // Get UAVObjectManager instance
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    objMngr = pm->getObject<UAVObjectManager>();

    settings = pm->getObject<Core::Internal::GeneralSettings>();
}

TelemetryManager::~TelemetryManager()
{
}

void TelemetryManager::start(QIODevice *dev)
{
    utalk = new UAVTalk(dev, objMngr);
    telemetry = new Telemetry(utalk, objMngr);
    telemetryMon = new TelemetryMonitor(objMngr, telemetry);
    connect(telemetryMon, &TelemetryMonitor::connected, this, &TelemetryManager::onConnect);
    connect(telemetryMon, &TelemetryMonitor::disconnected, this, &TelemetryManager::onDisconnect);
}

void TelemetryManager::stop()
{
    telemetryMon->disconnect(this);
    telemetryMon->deleteLater();
    telemetryMon = NULL;
    telemetry->deleteLater();
    telemetry = NULL;
    utalk->deleteLater();
    utalk = NULL;
    onDisconnect();
}

void TelemetryManager::onConnect()
{
    m_connected = true;
    emit connected();
    emit connectedChanged(m_connected);
}

void TelemetryManager::onDisconnect()
{
    m_connected = false;
    emit disconnected();
    emit connectedChanged(m_connected);
}

QByteArray *TelemetryManager::downloadFile(quint32 fileId, quint32 maxSize,
        std::function<void(quint32)>progressCb)
{
    if (!telemetry) {
        return NULL;
    }

    return telemetry->downloadFile(fileId, maxSize, progressCb);
}
