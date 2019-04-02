/**
******************************************************************************
* @file       matekplugin.cpp
* @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
*
* @addtogroup GCSPlugins GCS Plugins
* @{
* @addtogroup Boards_MatekPlugin Matek boards support Plugin
* @{
* @brief Plugin to support Matek boards
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

#include "matekplugin.h"
#include "matek405.h"
#include <QtPlugin>

MatekPlugin::MatekPlugin()
{
    // Do nothing
}

MatekPlugin::~MatekPlugin()
{
    // Do nothing
}

bool MatekPlugin::initialize(const QStringList &args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);
    return true;
}

void MatekPlugin::extensionsInitialized()
{
    /**
     * Create the board objects here.
     *
     */
    MATEK405 *matek405 = new MATEK405();
    addAutoReleasedObject(matek405);
}

void MatekPlugin::shutdown()
{
}
