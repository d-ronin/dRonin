/**
 ******************************************************************************
 *
 * @file       uavdataobject.cpp
 *
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 *
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "uavdataobject.h"

/**
 * Constructor
 */
UAVDataObject::UAVDataObject(quint32 objID, bool isSingleInst, bool isSet, const QString &name)
    : UAVObject(objID, isSingleInst, name)
{
    mobj = NULL;
    this->isSet = isSet;
    this->isPresentOnHardware = unknownPresent;
}

/**
 * Initialize instance ID and assign a metaobject
 */
void UAVDataObject::initialize(quint32 instID, UAVMetaObject *mobj)
{
    this->mobj = mobj;
    UAVObject::initialize(instID);
}

/**
 * Assign a metaobject
 */
void UAVDataObject::initialize(UAVMetaObject *mobj)
{
    this->mobj = mobj;
}

/**
 * Returns true if this is a data object holding module settings
 */
bool UAVDataObject::isSettings()
{
    return isSet;
}

/**
 * Set the object's metadata
 */
void UAVDataObject::setMetadata(const Metadata &mdata)
{
    if (mobj != NULL) {
        mobj->setData(mdata);
    }
}

/**
 * Get the object's metadata
 */
UAVObject::Metadata UAVDataObject::getMetadata(void)
{
    if (mobj != NULL) {
        return mobj->getData();
    } else {
        return getDefaultMetadata();
    }
}

/**
 * Get the metaobject
 */
UAVMetaObject *UAVDataObject::getMetaObject()
{
    return mobj;
}

bool UAVDataObject::getIsPresentOnHardware() const
{
    return isPresentOnHardware == isPresent;
}

void UAVDataObject::setIsPresentOnHardware(bool value)
{
    bool temp = getIsPresentOnHardware();

    if (value) {
        isPresentOnHardware = isPresent;
    } else {
        isPresentOnHardware = notPresent;
    }

    if (temp != getIsPresentOnHardware()) {
        emit presentOnHardwareChanged(this);
        emit presentOnHardwareChanged(isPresentOnHardware);
    }
}

void UAVDataObject::resetIsPresentOnHardware()
{
    enum presence old = isPresentOnHardware;

    isPresentOnHardware = unknownPresent;

    if (old == isPresent) {
        emit presentOnHardwareChanged(this);
        emit presentOnHardwareChanged(true);
    }
}
