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

// Number of retries for initial session object fetching
// This is needed because sometimes the object is lost when asked right uppon connection
#define SESSION_INIT_RETRIES 3
// Delay between initial session object fetching retries (number of times defined above)
#define SESSION_INITIAL_RETRIEVE_TIMEOUT 500
// Timeout for the all session negotiation, the system will go to failsafe after it
#define SESSION_RETRIEVE_TIMEOUT 20000
// Number of retries for the session object fetching during negotiation
#define SESSION_OBJ_RETRIEVE_RETRIES 3
// Timeout for the object fetching fase, the system will stop fetching objects and emit connected
// after this
#define OBJECT_RETRIEVE_TIMEOUT 7000
// IAP object is very important, retry if not able to get it the first time
#define IAP_OBJECT_RETRIES 3

#ifdef TELEMETRYMONITOR_DEBUG
#define TELEMETRYMONITOR_QXTLOG_DEBUG(...) qDebug() << __VA_ARGS__
#else // TELEMETRYMONITOR_DEBUG
#define TELEMETRYMONITOR_QXTLOG_DEBUG(...)
#endif // TELEMETRYMONITOR_DEBUG

/**
 * Constructor
 */
TelemetryMonitor::TelemetryMonitor(UAVObjectManager *objMngr, Telemetry *tel,
                                   QHash<quint16, QList<objStruc>> sessions)
    : connectionStatus(CON_DISCONNECTED)
    , objMngr(objMngr)
    , tel(tel)
    , numberOfObjects(0)
    , retries(0)
    , requestsInFlight(0)
    , isManaged(true)
    , sessions(sessions)
{
    sessionID = QDateTime::currentDateTime().toTime_t();
    this->connectionTimer = new QTime();
    // Get stats objects
    gcsStatsObj = GCSTelemetryStats::GetInstance(objMngr);
    flightStatsObj = FlightTelemetryStats::GetInstance(objMngr);

    sessionObj = SessionManaging::GetInstance(objMngr);

    // Listen for flight stats updates
    connect(flightStatsObj, &UAVObject::objectUpdated, this, &TelemetryMonitor::flightStatsUpdated);

    // Start update timer
    statsTimer = new QTimer(this);
    objectRetrieveTimeout = new QTimer(this);
    objectRetrieveTimeout->setSingleShot(true);
    sessionInitialRetrieveTimeout = new QTimer(this);
    sessionInitialRetrieveTimeout->setSingleShot(true);
    connect(statsTimer, &QTimer::timeout, this, &TelemetryMonitor::processStatsUpdates);
    connect(sessionInitialRetrieveTimeout, &QTimer::timeout, this,
            &TelemetryMonitor::sessionInitialRetrieveTimeoutCB);
    connect(objectRetrieveTimeout, &QTimer::timeout, this,
            &TelemetryMonitor::objectRetrieveTimeoutCB);
    statsTimer->start(STATS_CONNECT_PERIOD_MS);

    Core::ConnectionManager *cm = Core::ICore::instance()->connectionManager();
    connect(this, &TelemetryMonitor::connected, cm, &Core::ConnectionManager::telemetryConnected);
    connect(this, &TelemetryMonitor::disconnected, cm,
            &Core::ConnectionManager::telemetryDisconnected);
    connect(this, &TelemetryMonitor::telemetryUpdated, cm,
            &Core::ConnectionManager::telemetryUpdated);
    connect(sessionObj, &UAVObject::objectUnpacked, this, &TelemetryMonitor::sessionObjUnpackedCB);
    connect(objMngr, &UAVObjectManager::newInstance, this, &TelemetryMonitor::newInstanceSlot);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    settings = pm->getObject<Core::Internal::GeneralSettings>();
}

TelemetryMonitor::~TelemetryMonitor()
{
    // Before saying goodbye, set the GCS connection status to disconnected too:
    GCSTelemetryStats::DataFields gcsStats = gcsStatsObj->getData();
    gcsStats.Status = GCSTelemetryStats::STATUS_DISCONNECTED;
    if (settings->useSessionManaging()) {
        foreach (UAVObjectManager::ObjectMap map, objMngr->getObjects()) {
            foreach (UAVObject *obj, map.values()) {
                UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
                if (dobj)
                    dobj->setIsPresentOnHardware(false);
            }
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
    // Get all objects, add metaobjects, settings and data objects with OnChange update mode to the
    // queue
    queue.clear();
    retries = 0;
    objectRetrieveTimeout->start(OBJECT_RETRIEVE_TIMEOUT);
    foreach (UAVObjectManager::ObjectMap map, objMngr->getObjects().values()) {
        UAVObject *obj = map.first();
        if (obj->getObjID() == SessionManaging::OBJID) {
            continue;
        }
        UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
        UAVObject::Metadata mdata = obj->getMetadata();
        if (dobj != NULL) {
            if (!dobj->getIsPresentOnHardware()) {
                TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 %1 not present on hardware, skipping")
                                                  .arg(Q_FUNC_INFO)
                                                  .arg(obj->getName()));
                continue;
            }
            queue.enqueue(dobj->getMetaObject());
            if (dobj->isSettings()) {
                TELEMETRYMONITOR_QXTLOG_DEBUG(
                    QString("%0 queing settings object %1").arg(Q_FUNC_INFO).arg(dobj->getName()));
                queue.enqueue(obj);
            } else {
                if (UAVObject::GetFlightTelemetryUpdateMode(mdata)
                    == UAVObject::UPDATEMODE_ONCHANGE) {
                    TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 queing UPDATEMODE_ONCHANGE object %1")
                                                      .arg(Q_FUNC_INFO)
                                                      .arg(dobj->getName()));
                    queue.enqueue(obj);
                } else {
                    TELEMETRYMONITOR_QXTLOG_DEBUG(
                        QString("%0 skipping %1, not settings nor metadata or UPDATEMODE_ONCHANGE")
                            .arg(Q_FUNC_INFO)
                            .arg(dobj->getName()));
                }
            }
        }
    }
    // Start retrieving
    TELEMETRYMONITOR_QXTLOG_DEBUG(
        QString(
            tr("Starting to retrieve meta and settings objects from the autopilot (%1 objects)"))
            .arg(queue.length()));
    retrieveNextObject();
}

void TelemetryMonitor::changeObjectInstances(quint32 objID, quint32 instID, bool delayed)
{
    TELEMETRYMONITOR_QXTLOG_DEBUG(
        QString("%0 OBJID:%1 INSTID:%2").arg(Q_FUNC_INFO).arg(objID).arg(instID));
    UAVObject *obj = objMngr->getObject(objID);
    if (obj) {
        UAVDataObject *dobj = NULL;
        dobj = dynamic_cast<UAVDataObject *>(obj);
        if (dobj == NULL) {
            return;
        }
        if (dobj->isSingleInstance()) {
            return;
        }
        quint32 currentInstances = objMngr->getNumInstances(objID);
        if (currentInstances != instID) {
            TELEMETRYMONITOR_QXTLOG_DEBUG(
                QString("%0 Object %1 has %2 instances on hw and %3 on the GCS")
                    .arg(Q_FUNC_INFO)
                    .arg(dobj->getName())
                    .arg(sessionObj->getObjectInstances())
                    .arg(currentInstances));
            if (currentInstances < sessionObj->getObjectInstances()) {
                TELEMETRYMONITOR_QXTLOG_DEBUG(
                    QString("%0 cloned and registered object %1 INSTID=%2")
                        .arg(Q_FUNC_INFO)
                        .arg(dobj->getName())
                        .arg(instID - 1));
                UAVDataObject *instobj = dobj->clone(instID - 1);
                objMngr->registerObject(instobj);
            } else {
                TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 unregistered object %1 INSTID=%2")
                                                  .arg(Q_FUNC_INFO)
                                                  .arg(dobj->getName())
                                                  .arg(instID));
                UAVObject *obji = objMngr->getObject(objID, instID);
                UAVDataObject *dobji = dynamic_cast<UAVDataObject *>(obji);
                if (dobji) {
                    objMngr->unRegisterObject(dobji);
                }
            }
        }
        for (int x = 0; x < objMngr->getNumInstances(objID); ++x) {
            UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(objMngr->getObject(objID, x));
            if (dobj) {
                if (delayed)
                    delayedUpdate.append(dobj);
                else
                    dobj->setIsPresentOnHardware(true);
            }
        }
    }
}

/**
 * Retrieve the next object in the queue
 */
void TelemetryMonitor::retrieveNextObject()
{
    // If queue is empty return
    if (queue.isEmpty() && requestsInFlight == 0) {
        TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 Object retrieval completed").arg(Q_FUNC_INFO));
        if (isManaged) {
            TELEMETRYMONITOR_QXTLOG_DEBUG(
                QString("%0 connectionStatus set to CON_CONNECTED_MANAGED( %1 )")
                    .arg(Q_FUNC_INFO)
                    .arg(connectionStatus));
            connectionStatus = CON_CONNECTED_MANAGED;
        } else {
            TELEMETRYMONITOR_QXTLOG_DEBUG(
                QString("%0 connectionStatus set to CON_CONNECTED_MANAGED( %1 )")
                    .arg(Q_FUNC_INFO)
                    .arg(connectionStatus));
            connectionStatus = CON_CONNECTED_UNMANAGED;
        }
        foreach (UAVDataObject *uavo, delayedUpdate) {
            uavo->setIsPresentOnHardware(true);
        }
        delayedUpdate.clear();
        emit connected();
        sessionInitialRetrieveTimeout->stop();
        objectRetrieveTimeout->stop();
        return;
    }

    objectRetrieveTimeout->stop();
    objectRetrieveTimeout->start(OBJECT_RETRIEVE_TIMEOUT);

    while (requestsInFlight < MAX_REQUESTS_IN_FLIGHT) {
        if (queue.isEmpty()) {
            return;
        }

        // Get next object from the queue
        UAVObject *obj = queue.dequeue();
        // Connect to object
        TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 requestiong %1 from board INSTID:%2")
                                          .arg(Q_FUNC_INFO)
                                          .arg(obj->getName())
                                          .arg(obj->getInstID()));

        requestsInFlight++;

        connect(obj, QOverload<UAVObject *, bool>::of(&UAVObject::transactionCompleted), this,
                &TelemetryMonitor::transactionCompleted);
        // Request update
        obj->requestUpdateAllInstances();
    }
}

/**
 * Called by the retrieved object when a transaction is completed.
 */
void TelemetryMonitor::transactionCompleted(UAVObject *obj, bool success)
{
    TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 received %1 OBJID:%2 result:%3")
                                      .arg(Q_FUNC_INFO)
                                      .arg(obj->getName())
                                      .arg(obj->getObjID())
                                      .arg(success));
    Q_UNUSED(success);
    if (obj->getObjID() == FirmwareIAPObj::OBJID) {
        if (!success && (retries < IAP_OBJECT_RETRIES)) {
            ++retries;
            obj->requestUpdate();

            return;
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
        TELEMETRYMONITOR_QXTLOG_DEBUG(
            QString("%0 connection lost while retrieving objects, stopped object retrievel")
                .arg(Q_FUNC_INFO));
        queue.clear();
        objectRetrieveTimeout->stop();
        sessionInitialRetrieveTimeout->stop();
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

void TelemetryMonitor::sessionObjUnpackedCB(UAVObject *obj)
{
    sessionObjRetries = 0;
    switch (connectionStatus) {
    case CON_INITIALIZING:
        if (sessions.contains(sessionObj->getSessionID())
            && (sessions.value(sessionObj->getSessionID()).count()
                == sessionObj->getNumberOfObjects())) {
            sessionID = sessionObj->getSessionID();
            numberOfObjects = sessionObj->getNumberOfObjects();
            TELEMETRYMONITOR_QXTLOG_DEBUG(
                QString("%0 status:%1 session already known startRetrievingObjects")
                    .arg(Q_FUNC_INFO)
                    .arg(connectionStatus));
            foreach (objStruc objs, sessions.value(sessionObj->getSessionID())) {
                UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(objMngr->getObject(objs.objID));
                if (dobj) {
                    if (dobj->isSingleInstance()) {
                        dobj->setIsPresentOnHardware(true);
                    } else {
                        changeObjectInstances(objs.objID, objs.instID, true);
                    }
                }
            }
            startRetrievingObjects();
        } else {
            TELEMETRYMONITOR_QXTLOG_DEBUG(
                QString("%0 new session startSessionRetrieving HWsessionID=%1 GCSsessionID=%2 "
                        "HWnumberofobjs=%3 GCSnumberofobjs=%4")
                    .arg(Q_FUNC_INFO)
                    .arg(sessionObj->getSessionID())
                    .arg(sessionID)
                    .arg(sessionObj->getNumberOfObjects())
                    .arg(sessions.value(sessionObj->getSessionID()).count()));
            startSessionRetrieving(NULL);
        }
        break;
    case CON_CONNECTED_MANAGED:
        qDebug() << QString("HW=%0 GCS=%1 GCS_ADJ=%2")
                        .arg(sessionObj->getSessionID())
                        .arg(sessionID)
                        .arg(sessionID + 1);
        if (sessionObj->getSessionID() == sessionID
            && numberOfObjects == sessionObj->getNumberOfObjects())
            return;
        else if (sessionObj->getSessionID() == (sessionID + 1)) // number of instances changed
        {
            TELEMETRYMONITOR_QXTLOG_DEBUG(
                QString("%0 received sessionID shift by one, this signal that a multi instance "
                        "object changed number of instances")
                    .arg(Q_FUNC_INFO));
            changeObjectInstances(sessionObj->getObjectID(), sessionObj->getObjectInstances(),
                                  false);
            sessionID = sessionID + 1;
            saveSession();
        } else if (!sessions.contains(sessionObj->getSessionID())) {
            TELEMETRYMONITOR_QXTLOG_DEBUG(
                QString("%0 new session during CONNECTED? startSessionRetrieving")
                    .arg(Q_FUNC_INFO));
            startSessionRetrieving(NULL);
        }
        break;
    case CON_SESSION_INITIALIZING:
        startSessionRetrieving(obj);
        break;
    case CON_RETRIEVING_OBJECTS:
        TELEMETRYMONITOR_QXTLOG_DEBUG(
            QString(
                "%0 received sessionManaging object during object retrievel, this shouldn't happen")
                .arg(Q_FUNC_INFO));
        break;
    case CON_CONNECTED_UNMANAGED:
    case CON_DISCONNECTED:
        break;
    }
}

void TelemetryMonitor::objectRetrieveTimeoutCB()
{
    queue.clear();
}

void TelemetryMonitor::sessionInitialRetrieveTimeoutCB()
{
    if ((connectionStatus == CON_INITIALIZING) || (connectionStatus == CON_SESSION_INITIALIZING)) {
        if (sessionObjRetries < SESSION_OBJ_RETRIEVE_RETRIES) {
            TELEMETRYMONITOR_QXTLOG_DEBUG(
                QString("%0 session retrieve timeout try=%1, going to retry")
                    .arg(Q_FUNC_INFO)
                    .arg(sessionObjRetries));
            ++sessionObjRetries;
            sessionObj->updated();
            sessionInitialRetrieveTimeout->start(SESSION_INITIAL_RETRIEVE_TIMEOUT);
        }
    }
}

void TelemetryMonitor::saveSession()
{
    if (isManaged) {
        QList<objStruc> list;
        foreach (UAVObjectManager::ObjectMap map, objMngr->getObjects().values()) {
            foreach (UAVObject *obj, map) {
                UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
                if (dobj) {
                    if (dobj->getIsPresentOnHardware() && dobj->getInstID() == 0) {
                        objStruc objs;
                        objs.objID = dobj->getObjID();
                        objs.instID = objMngr->getNumInstances(obj->getObjID());
                        list.append(objs);
                    }
                }
            }
        }
        sessions.insert(sessionID, list);
    }
}

void TelemetryMonitor::newInstanceSlot(UAVObject *obj)
{
    UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
    if (!dobj)
        return;
    if (!isManaged)
        dobj->setIsPresentOnHardware(true);
}

void TelemetryMonitor::startSessionRetrieving(UAVObject *session)
{
    static int currentIndex = 0;
    static int objectCount = 0;
    static int sessionNegotiationRetries = 0;
    if (session == NULL) {
        TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 NULL new session start").arg(Q_FUNC_INFO));
        foreach (UAVObjectManager::ObjectMap map, objMngr->getObjects()) {
            foreach (UAVObject *obj, map.values()) {
                UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
                if (dobj)
                    dobj->setIsPresentOnHardware(false);
            }
        }
        currentIndex = 0;
        objectCount = 0;
        sessionObjRetries = 0;
        connectionStatus = CON_SESSION_INITIALIZING;
        sessionObj->setSessionID(0);
        sessionObj->updated();
    } else if (sessionObj->getSessionID() == 0) {
        objectCount = sessionObj->getNumberOfObjects();
        if (objectCount == 0)
            return;
        sessionID = QDateTime::currentDateTime().toTime_t();
        sessionObj->setSessionID(sessionID);
        sessionObj->setObjectOfInterestIndex(0);
        TELEMETRYMONITOR_QXTLOG_DEBUG(
            QString("%0 SESSION SETUP with numberofobjects:%1 and sessionID:%2")
                .arg(Q_FUNC_INFO)
                .arg(objectCount)
                .arg(sessionID));
        sessionObj->updated();
    } else if (sessionObj->getSessionID() == sessionID) {
        if (sessionObj->getObjectOfInterestIndex() != currentIndex) {
            TELEMETRYMONITOR_QXTLOG_DEBUG(
                QString("%0 RECEIVED WRONG INDEX RETRY current retries:%1")
                    .arg(Q_FUNC_INFO)
                    .arg(sessionNegotiationRetries));
            if (sessionNegotiationRetries < SESSION_INIT_RETRIES) {
                sessionObj->setObjectOfInterestIndex(currentIndex);
                ++sessionNegotiationRetries;
                sessionObj->updated();
                return;
            }
        } else {
            sessionNegotiationRetries = 0;
        }
        UAVObject *obj = objMngr->getObject(sessionObj->getObjectID());
        if (obj) {
            TELEMETRYMONITOR_QXTLOG_DEBUG(QString("%0 RECEIVED INDEX:%1 object:%2 instances:%3")
                                              .arg(Q_FUNC_INFO)
                                              .arg(sessionObj->getObjectOfInterestIndex())
                                              .arg(obj->getName())
                                              .arg(sessionObj->getObjectInstances()));
            UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
            if (dobj) {
                dobj->setIsPresentOnHardware(true);
            }
            if ((sessionObj->getObjectInstances() > 1) && !obj->isSingleInstance()) {
                changeObjectInstances(dobj->getObjID(), sessionObj->getObjectInstances(), true);
            }
        }
        ++currentIndex;
        if (currentIndex < objectCount) {
            TELEMETRYMONITOR_QXTLOG_DEBUG(
                QString("%0 ASKING BOARD FOR INDEX:%1").arg(Q_FUNC_INFO).arg(currentIndex));
            sessionObj->setObjectOfInterestIndex(currentIndex);
            sessionObj->updated();
        } else {
            saveSession();
            startRetrievingObjects();
        }
    }
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
        sessionInitialRetrieveTimeout->start(SESSION_INITIAL_RETRIEVE_TIMEOUT);
        sessionObj->requestUpdate();
        isManaged = true;
    }
    if (gcsStats.Status == GCSTelemetryStats::STATUS_DISCONNECTED && gcsStats.Status != oldStatus) {
        statsTimer->setInterval(STATS_CONNECT_PERIOD_MS);
        connectionStatus = CON_DISCONNECTED;
        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
        Core::Internal::GeneralSettings *settings =
            pm->getObject<Core::Internal::GeneralSettings>();
        if (settings->useSessionManaging()) {
            foreach (UAVObjectManager::ObjectMap map, objMngr->getObjects()) {
                foreach (UAVObject *obj, map.values()) {
                    UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
                    if (dobj)
                        dobj->setIsPresentOnHardware(false);
                }
            }
        }
        emit disconnected();
    }
}
