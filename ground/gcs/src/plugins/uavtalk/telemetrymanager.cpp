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
    : autopilotConnected(false)
{
    // Get UAVObjectManager instance
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    objMngr = pm->getObject<UAVObjectManager>();

    settings = pm->getObject<Core::Internal::GeneralSettings>();
    connect(settings, &Core::Internal::GeneralSettings::generalSettingsChanged, this,
            &TelemetryManager::onGeneralSettingsChanged);
    connect(pm, &ExtensionSystem::PluginManager::pluginsLoadEnded, this,
            &TelemetryManager::onGeneralSettingsChanged);
}

TelemetryManager::~TelemetryManager()
{
}

bool TelemetryManager::isConnected()
{
    return autopilotConnected;
}

void TelemetryManager::start(QIODevice *dev)
{
    utalk = new UAVTalk(dev, objMngr);
    telemetry = new Telemetry(utalk, objMngr);
    telemetryMon = new TelemetryMonitor(objMngr, telemetry, sessions);
    connect(telemetryMon, &TelemetryMonitor::connected, this, &TelemetryManager::onConnect);
    connect(telemetryMon, &TelemetryMonitor::disconnected, this, &TelemetryManager::onDisconnect);
}

void TelemetryManager::stop()
{
    telemetryMon->disconnect(this);
    sessions = telemetryMon->savedSessions();
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
    autopilotConnected = true;
    emit connected();
}

void TelemetryManager::onDisconnect()
{
    autopilotConnected = false;
    emit disconnected();
}

void TelemetryManager::onGeneralSettingsChanged()
{
    if (!settings->useSessionManaging()) {
        foreach (UAVObjectManager::ObjectMap map, objMngr->getObjects()) {
            foreach (UAVObject *obj, map.values()) {
                UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
                if (dobj) {
                    dobj->setIsPresentOnHardware(false);
                    dobj->setIsPresentOnHardware(true);
                }
            }
        }
    }
}
