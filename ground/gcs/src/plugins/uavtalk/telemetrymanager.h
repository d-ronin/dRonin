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

#ifndef TELEMETRYMANAGER_H
#define TELEMETRYMANAGER_H

#include "uavtalk_global.h"
#include <coreplugin/generalsettings.h>
#include <extensionsystem/pluginmanager.h>
#include "telemetrymonitor.h"
#include "telemetry.h"
#include "uavtalk.h"
#include "uavobjectmanager.h"
#include <QIODevice>
#include <QObject>

class UAVTALK_EXPORT TelemetryManager : public QObject
{
    Q_OBJECT

public:
    TelemetryManager();
    ~TelemetryManager();

    void start(QIODevice *dev);
    void stop();
    bool isConnected();

signals:
    void connected();
    void disconnected();

private slots:
    void onConnect();
    void onDisconnect();
    void onGeneralSettingsChanged();

private:
    UAVObjectManager *objMngr;
    UAVTalk *utalk;
    Telemetry *telemetry;
    TelemetryMonitor *telemetryMon;

    bool autopilotConnected;
    QHash<quint16, QList<TelemetryMonitor::objStruc>> sessions;
    Core::Internal::GeneralSettings *settings;
};

#endif // TELEMETRYMANAGER_H
