/**
 ******************************************************************************
 * @file       taulink.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_TauLabsPlugin Tau Labs boards support Plugin
 * @{
 * @brief Plugin to support boards by the Tau Labs project
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
#ifndef TAULINK_H
#define TAULINK_H

#include <coreplugin/iboardtype.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include "hwtaulink.h"

class IBoardType;

class TauLink : public Core::IBoardType
{
public:
    TauLink();
    virtual ~TauLink();

    virtual QString shortName();
    virtual QString boardDescription();
    virtual bool queryCapabilities(BoardCapabilities capability);
    virtual QPixmap getBoardPicture();
    virtual QString getHwUAVO();
    HwTauLink *getSettings();

private:
    UAVObjectUtilManager *uavoUtilManager;
};

#endif // TAULINK_H
