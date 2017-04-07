/**
 ******************************************************************************
 * @file       modeluavproxy.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup OPMapPlugin OpenPilot Map Plugin
 * @{
 * @brief The OpenPilot Map plugin
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
#ifndef ModelUavoProxy_H
#define ModelUavoProxy_H

#include "flightdatamodel.h"
#include "modeluavoproxy.h"
#include "waypoint.h"
#include <QObject>

class ModelUavoProxy : public QObject
{
    Q_OBJECT
public:
    explicit ModelUavoProxy(QObject *parent, FlightDataModel *model);

private:
    //! Robustly upload a waypoint (like smart save)
    bool robustUpdate(Waypoint::DataFields data, int instance);

    //! Fetch the home LLA position
    bool getHomeLocation(double *homeLLA);

public slots:
    //! Cast from the internal representation to the UAVOs
    bool modelToObjects();

    //! Cast from the UAVOs to the internal representation
    void objectsToModel();

    //! Whenever a waypoint transaction is completed
    void waypointTransactionCompleted(UAVObject *, bool);

signals:
    void waypointTransactionSucceeded();
    void waypointTransactionFailed();
    void sendPathPlanToUavProgress(int percent);

private:
    UAVObjectManager *objManager;
    Waypoint *waypointObj;
    FlightDataModel *myModel;

    //! Track if each waypoint was updated
    QMap<int, bool> waypointTransactionResult;
};

#endif // ModelUavoProxy_H
