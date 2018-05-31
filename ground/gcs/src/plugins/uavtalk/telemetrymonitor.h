/**
 ******************************************************************************
 *
 * @file       telemetrymonitor.cpp
 *
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 *
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef TELEMETRYMONITOR_H
#define TELEMETRYMONITOR_H

#include <queue>

#include <QObject>
#include <QTimer>
#include <QTime>
#include "uavobjects/uavobjectmanager.h"
#include "gcstelemetrystats.h"
#include "flighttelemetrystats.h"
#include "systemstats.h"
#include "telemetry.h"
#include <coreplugin/generalsettings.h>
#include <extensionsystem/pluginmanager.h>

class TelemetryMonitor : public QObject
{
    Q_OBJECT

public:
    struct objStruc
    {
        quint32 objID;
        quint32 instID;
    };

    TelemetryMonitor(UAVObjectManager *objMngr, Telemetry *tel);
    ~TelemetryMonitor();

signals:
    void connected();
    void disconnected();
    void telemetryUpdated(double txRate, double rxRate);

public slots:
    void transactionCompleted(UAVObject *obj, bool success, bool nacked);
    void processStatsUpdates();
    void flightStatsUpdated(UAVObject *obj);
private slots:
    void objectRetrieveTimeoutCB();
    void newInstanceSlot(UAVObject *);

private:
    static const int STATS_UPDATE_PERIOD_MS = 1600;
    static const int STATS_CONNECT_PERIOD_MS = 350;
    static const int CONNECTION_TIMEOUT_MS = 8000;
    static const int MAX_REQUESTS_IN_FLIGHT = 3;
    enum connectionStatusEnum {
        CON_DISCONNECTED,
        CON_INITIALIZING,
        CON_RETRIEVING_OBJECTS,
        CON_CONNECTED_MANAGED
    };

    typedef bool (*queueCompareFunc)(UAVObject *, UAVObject *);
    static bool queueCompare(UAVObject *left, UAVObject *right);
    connectionStatusEnum connectionStatus;
    UAVObjectManager *objMngr;
    Telemetry *tel;
    std::priority_queue<UAVObject *, std::vector<UAVObject *>, queueCompareFunc> queue;
    GCSTelemetryStats *gcsStatsObj;
    FlightTelemetryStats *flightStatsObj;
    QTimer *statsTimer;
    QTime *connectionTimer;
    QTimer *objectRetrieveTimeout;
    int requestsInFlight;

    void startRetrievingObjects();
    void retrieveNextObject();
};

#endif // TELEMETRYMONITOR_H
