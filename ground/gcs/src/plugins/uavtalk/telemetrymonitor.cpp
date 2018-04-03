/**
 ******************************************************************************
 *
 * @file       telemetrymonitor.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
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

#include "telemetrymonitor.h"
#include "coreplugin/connectionmanager.h"
#include "coreplugin/icore.h"
#include "firmwareiapobj.h"

// Timeout for the object fetching phase, the system will stop fetching objects and emit connected
// after this
#define OBJECT_RETRIEVE_TIMEOUT 20000
// IAP object is very important, retry if not able to get it the first time
#define IAP_OBJECT_RETRIES 3

#ifdef TELEMETRYMONITOR_DEBUG
#define TELEMETRYMONITOR_QXTLOG_DEBUG(...) qDebug() << __VA_ARGS__
#else // TELEMETRYMONITOR_DEBUG
#define TELEMETRYMONITOR_QXTLOG_DEBUG(...)
#endif // TELEMETRYMONITOR_DEBUG

bool TelemetryMonitor::queueCompare(UAVObject *left, UAVObject *right)
{
    /* Should return true if right should be dequeued before left. */

    /* It's advantageous to dequeue multi-instance objs first, so if
     * there's many IDs we can be doing the round trips while other
     * objects arrive (instead of getting stuck round-trip bound */

    if (left->isSingleInstance() && (!right->isSingleInstance())) {
        return true;
    } else if ((!left->isSingleInstance() && right->isSingleInstance())) {
        return false;
    }

    /* It's advantageous to dequeue metaobjects last, because we
     * probably know about their existance by this stage. */
    UAVDataObject *leftD = dynamic_cast<UAVDataObject *>(left);
    UAVDataObject *rightD = dynamic_cast<UAVDataObject *>(right);

    if (leftD && (!rightD)) {
        /* Right is a metaobj */
        return false;
    } else if ((!leftD) && (rightD)) {
        /* Right is NOT a metaobj */
        return true;
    }

    /* At this point either both are meta or both are not meta */
    if (leftD) {
        /* It's advantageous to dequeue settings next, since they're
         * not auto-telemetered and we won't find out about them by
         * serendipity */
        if ((!leftD->isSettings()) && (rightD->isSettings())) {
            return true;
        } else if ((leftD->isSettings()) && (!rightD->isSettings())) {
            return false;
        }
    }

    /* Fall back to name last, because it's pleasing to retrieve in order */
    return left->getName() > right->getName();
}

/**
 * Constructor
 */
TelemetryMonitor::TelemetryMonitor(UAVObjectManager *objMngr, Telemetry *tel)
    : connectionStatus(CON_DISCONNECTED)
    , objMngr(objMngr)
    , tel(tel)
    , queue(decltype(queue)(queueCompare))
    , requestsInFlight(0)
{
    this->connectionTimer = new QTime();
    // Get stats objects
    gcsStatsObj = GCSTelemetryStats::GetInstance(objMngr);
    flightStatsObj = FlightTelemetryStats::GetInstance(objMngr);

    // Listen for flight stats updates
    connect(flightStatsObj, &UAVObject::objectUpdated, this, &TelemetryMonitor::flightStatsUpdated);

    // Start update timer
    statsTimer = new QTimer(this);
    objectRetrieveTimeout = new QTimer(this);
    objectRetrieveTimeout->setSingleShot(true);
    connect(statsTimer, &QTimer::timeout, this, &TelemetryMonitor::processStatsUpdates);
    connect(objectRetrieveTimeout, &QTimer::timeout, this,
            &TelemetryMonitor::objectRetrieveTimeoutCB);
    statsTimer->start(STATS_CONNECT_PERIOD_MS);

    Core::ConnectionManager *cm = Core::ICore::instance()->connectionManager();
    connect(this, &TelemetryMonitor::connected, cm, &Core::ConnectionManager::telemetryConnected);
    connect(this, &TelemetryMonitor::disconnected, cm,
            &Core::ConnectionManager::telemetryDisconnected);
    connect(this, &TelemetryMonitor::telemetryUpdated, cm,
            &Core::ConnectionManager::telemetryUpdated);
    connect(objMngr, &UAVObjectManager::newInstance, this, &TelemetryMonitor::newInstanceSlot);
}

TelemetryMonitor::~TelemetryMonitor()
{
    // Before saying goodbye, set the GCS connection status to disconnected too:
    GCSTelemetryStats::DataFields gcsStats = gcsStatsObj->getData();
    gcsStats.Status = GCSTelemetryStats::STATUS_DISCONNECTED;

    foreach (UAVObjectManager::ObjectMap map, objMngr->getObjects()) {
        foreach (UAVObject *obj, map.values()) {
            UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
            if (dobj)
                dobj->resetIsPresentOnHardware();
        }
    }

    // Set data
    gcsStatsObj->setData(gcsStats);
}

/**
 * Initiate object retrieval, initialize queue with objects to be retrieved.
 */
void TelemetryMonitor::startRetrievingObjects()
{
    TELEMETRYMONITOR_QXTLOG_DEBUG(
        QString("%0 connectionStatus changed to CON_RETRIEVING_OBJECT").arg(Q_FUNC_INFO));
    connectionStatus = CON_RETRIEVING_OBJECTS;

    /* Clear the queue */
    queue = decltype(queue)(queueCompare);

    objectRetrieveTimeout->start(OBJECT_RETRIEVE_TIMEOUT);
    foreach (UAVObjectManager::ObjectMap map, objMngr->getObjects().values()) {
        UAVObject *obj = map.first();

        /* Enqueue everything; decide later whether to bother retrieving. */
        queue.push(obj);
    }

    // Start retrieving
    TELEMETRYMONITOR_QXTLOG_DEBUG(
        QString(
            tr("Starting to retrieve objects from the autopilot (%1 objects)"))
            .arg(queue.size()));
    retrieveNextObject();
}

/**
 * Retrieve the next object in the queue
 */
void TelemetryMonitor::retrieveNextObject()
{
    // If queue is empty return
    if (queue.empty() && requestsInFlight == 0) {
        TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 Object retrieval completed").arg(Q_FUNC_INFO));
        TELEMETRYMONITOR_QXTLOG_DEBUG(
            QString("%0 connectionStatus set to CON_CONNECTED_MANAGED( %1 )")
                .arg(Q_FUNC_INFO)
                .arg(connectionStatus));
        connectionStatus = CON_CONNECTED_MANAGED;
        emit connected();
        objectRetrieveTimeout->stop();
        return;
    }

    objectRetrieveTimeout->stop();
    objectRetrieveTimeout->start(OBJECT_RETRIEVE_TIMEOUT);

    while (requestsInFlight < MAX_REQUESTS_IN_FLIGHT) {
        if (queue.empty()) {
            return;
        }

        // Get next object from the queue
        UAVObject *obj = queue.top();
        queue.pop();

        UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);

        if (dobj) {
            if (dobj->getReceived() && dobj->isSingleInstance()) {
                /* We got it already.  That's easy! */
                continue;
            }

            if (dobj->getPresenceKnown() && (!dobj->getIsPresentOnHardware())) {
                TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 %1 not present on hardware, skipping")
                                                  .arg(Q_FUNC_INFO)
                                                  .arg(obj->getName()));
                continue;
            }
        } else {
            /* For metaobjects, skip if data object not there */
            UAVMetaObject *mobj = dynamic_cast<UAVMetaObject *>(obj);

            if (mobj) {
                UAVDataObject *pobj = dynamic_cast<UAVDataObject *>(
                        mobj->getParentObject());
                if (pobj->getPresenceKnown() && (!pobj->getIsPresentOnHardware())) {
                    TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 %1 not present on hardware, skipping associated meta obj")
                                                      .arg(Q_FUNC_INFO)
                                                      .arg(obj->getName()));
                    continue;
                }
            }
        }

        // Connect to object
        TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 requesting %1 from board INSTID:%2")
                                          .arg(Q_FUNC_INFO)
                                          .arg(obj->getName())
                                          .arg(obj->getInstID()));

        requestsInFlight++;

        connect(obj, QOverload<UAVObject *, bool, bool>::of(&UAVObject::transactionCompleted), this,
                &TelemetryMonitor::transactionCompleted);
        // Request update
        obj->requestUpdate();
    }
}

/**
 * Called by the retrieved object when a transaction is completed.
 */
void TelemetryMonitor::transactionCompleted(UAVObject *obj, bool success, bool nacked)
{
    TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 received %1 OBJID:%2 result:%3")
                                      .arg(Q_FUNC_INFO)
                                      .arg(obj->getName())
                                      .arg(obj->getObjID())
                                      .arg(success));
    Q_UNUSED(success);

    UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);

    if (nacked) {
        UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
        if (dobj) {
            if (dobj->isSingleInstance()) {
                dobj->setIsPresentOnHardware(false);
            } else {
                int instId = dobj->getInstID();

                if (instId <= 0) {
                    dobj->setIsPresentOnHardware(false);
                } else {
                    /* Multi-instance, shrink instance count down.  Inst
                     * ID must be 1, so getNumInstances should be 2 or
                     * larger */
                    int idx;

                    idx = objMngr->getNumInstances(dobj->getObjID());

                    while (idx > instId) {
                        idx--;
                        qInfo() << QString("Got nak instID %1 removing %2").arg(instId).arg(idx);
                        UAVObject *objR = objMngr->getObject(dobj->getObjID(),
                                idx);
                        UAVDataObject *dobjR = dynamic_cast<UAVDataObject *>(objR);
                        if (dobjR) {
                            objMngr->unRegisterObject(dobjR);
                        }
                    }
                }
            }
        }
    } else if (success) {
        if (dobj && (!dobj->isSingleInstance())) {
            /* multi-instance... temporarily register and enqueue next
             * instance
             */
            int new_idx = dobj->getInstID() + 1;

            UAVObject *instObj = objMngr->getObject(dobj->getObjID(),
                    new_idx);

            UAVDataObject *instdObj;
            if (instObj == NULL)  {
                instdObj = dobj->clone(new_idx);
                objMngr->registerObject(instdObj);
            } else {
                instdObj = dynamic_cast<UAVDataObject *>(instObj);
            }

            queue.push(instdObj);
        }
    }

    // Disconnect from sending object
    requestsInFlight--;
    obj->disconnect(this);

    // Process next object if telemetry is still available
    GCSTelemetryStats::DataFields gcsStats = gcsStatsObj->getData();
    if (gcsStats.Status == GCSTelemetryStats::STATUS_CONNECTED) {
        connectionStatus = CON_RETRIEVING_OBJECTS;

        retrieveNextObject();
    } else {
        qInfo() <<
            QString("%0 connection lost while retrieving objects, stopped object retrievel")
                .arg(Q_FUNC_INFO);
        /* Clear the queue */
        queue = decltype(queue)(queueCompare);

        objectRetrieveTimeout->stop();
        connectionStatus = CON_DISCONNECTED;
    }
}

/**
 * Called each time the flight stats object is updated by the autopilot
 */
void TelemetryMonitor::flightStatsUpdated(UAVObject *obj)
{
    Q_UNUSED(obj);

    // Force update if not yet connected
    GCSTelemetryStats::DataFields gcsStats = gcsStatsObj->getData();
    FlightTelemetryStats::DataFields flightStats = flightStatsObj->getData();
    if (gcsStats.Status != GCSTelemetryStats::STATUS_CONNECTED
        || flightStats.Status != FlightTelemetryStats::STATUS_CONNECTED) {
        processStatsUpdates();
    }
}

void TelemetryMonitor::objectRetrieveTimeoutCB()
{
    qInfo() <<
        QString("%0 reached timeout for object retrieval, clearing queue")
        .arg(Q_FUNC_INFO);

    /* Clear the queue */
    queue = decltype(queue)(queueCompare);
}

void TelemetryMonitor::newInstanceSlot(UAVObject *obj)
{
    UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
    if (!dobj)
        return;
    dobj->setIsPresentOnHardware(true);
}

/**
 * Called periodically to update the statistics and connection status.
 */
void TelemetryMonitor::processStatsUpdates()
{
    // Get telemetry stats
    GCSTelemetryStats::DataFields gcsStats = gcsStatsObj->getData();
    FlightTelemetryStats::DataFields flightStats = flightStatsObj->getData();
    Telemetry::TelemetryStats telStats = tel->getStats();

    // Update stats object
    gcsStats.RxDataRate = (float)telStats.rxBytes / ((float)statsTimer->interval() / 1000.0);
    gcsStats.TxDataRate = (float)telStats.txBytes / ((float)statsTimer->interval() / 1000.0);
    gcsStats.RxFailures += telStats.rxErrors;
    gcsStats.TxFailures += telStats.txErrors;
    gcsStats.TxRetries += telStats.txRetries;

    // Check for a connection timeout
    bool connectionTimeout;
    if (telStats.rxObjects > 0) {
        connectionTimer->start();
    }
    if (connectionTimer->elapsed() > CONNECTION_TIMEOUT_MS) {
        connectionTimeout = true;
    } else {
        connectionTimeout = false;
    }

    // Update connection state
    int oldStatus = gcsStats.Status;
    if (gcsStats.Status == GCSTelemetryStats::STATUS_DISCONNECTED) {
        // Request connection
        gcsStats.Status = GCSTelemetryStats::STATUS_HANDSHAKEREQ;
    } else if (gcsStats.Status == GCSTelemetryStats::STATUS_HANDSHAKEREQ) {
        // Check for connection acknowledge
        if (flightStats.Status == FlightTelemetryStats::STATUS_HANDSHAKEACK) {
            gcsStats.Status = GCSTelemetryStats::STATUS_CONNECTED;
        }
    } else if (gcsStats.Status == GCSTelemetryStats::STATUS_CONNECTED) {
        // Check if the connection is still active and the the autopilot is still connected
        if (flightStats.Status == FlightTelemetryStats::STATUS_DISCONNECTED || connectionTimeout) {
            gcsStats.Status = GCSTelemetryStats::STATUS_DISCONNECTED;
        }
    }

    emit telemetryUpdated((double)gcsStats.TxDataRate, (double)gcsStats.RxDataRate);

    // Set data
    gcsStatsObj->setData(gcsStats);

    // Force telemetry update if not yet connected
    if (gcsStats.Status != GCSTelemetryStats::STATUS_CONNECTED
        || flightStats.Status != FlightTelemetryStats::STATUS_CONNECTED) {
        gcsStatsObj->updated();
    }

    // Act on new connections or disconnections
    if (gcsStats.Status == GCSTelemetryStats::STATUS_CONNECTED && gcsStats.Status != oldStatus) {
        statsTimer->setInterval(STATS_UPDATE_PERIOD_MS);
        qDebug() << "Connection with the autopilot established";
        connectionStatus = CON_INITIALIZING;
        startRetrievingObjects();
    } else if (gcsStats.Status == GCSTelemetryStats::STATUS_DISCONNECTED && gcsStats.Status != oldStatus) {
        statsTimer->setInterval(STATS_CONNECT_PERIOD_MS);
        connectionStatus = CON_DISCONNECTED;
        foreach (UAVObjectManager::ObjectMap map, objMngr->getObjects()) {
            foreach (UAVObject *obj, map.values()) {
                UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
                if (dobj)
                    dobj->resetIsPresentOnHardware();
            }
        }

        emit disconnected();
    }
}
