/**
 ******************************************************************************
 *
 * @file       uavdataobject.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVObjectsPlugin UAVObjects Plugin
 * @{
 * @brief      The UAVUObjects GCS plugin
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
#ifndef UAVDATAOBJECT_H
#define UAVDATAOBJECT_H

#include "uavobjects/uavobjects_global.h"
#include "uavobjects/uavobject.h"
#include "uavobjects/uavobjectfield.h"
#include "uavobjects/uavmetaobject.h"
#include <QList>

class UAVOBJECTS_EXPORT UAVDataObject : public UAVObject
{
    Q_OBJECT
    Q_PROPERTY(bool isPresentOnHardware READ getIsPresentOnHardware WRITE setIsPresentOnHardware NOTIFY presentOnHardwareChanged)
public:
    UAVDataObject(quint32 objID, bool isSingleInst, bool isSet, const QString &name);
    void initialize(quint32 instID, UAVMetaObject *mobj);
    void initialize(UAVMetaObject *mobj);
    bool isSettings();
    void setMetadata(const Metadata &mdata);
    Metadata getMetadata();
    UAVMetaObject *getMetaObject();
    virtual UAVDataObject *clone(quint32 instID = 0) = 0;
    virtual UAVDataObject *dirtyClone() = 0;

    bool getIsPresentOnHardware() const;
    bool getPresenceKnown() const;
    void setReceived();
    bool getReceived();
    void setIsPresentOnHardware(bool value = true);
    void resetIsPresentOnHardware();

signals:
    void presentOnHardwareChanged(UAVDataObject *);
    void presentOnHardwareChanged(bool present);

private:
    UAVMetaObject *mobj;
    bool isSet;

    enum presence { notPresent, unknownPresent, isPresent, isPresentAndReceived }
                isPresentOnHardware;
};

#endif // UAVDATAOBJECT_H
