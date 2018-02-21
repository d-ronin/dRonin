/**
 ******************************************************************************
 * @file       rcexplorerplugin.cpp
 * @author     Dronin, http://dronin.org, Copyright (C) 2016
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_RcExplorerPlugin RcExplorer boards support Plugin
 * @{
 * @brief Plugin to support boards by RcExplorer
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "rcexplorerplugin.h"
#include "f3fc.h"
#include <QtPlugin>


F3FcPlugin::F3FcPlugin()
{
   // Do nothing
}

F3FcPlugin::~F3FcPlugin()
{
   // Do nothing
}

bool F3FcPlugin::initialize(const QStringList& args, QString *errMsg)
{
   Q_UNUSED(args);
   Q_UNUSED(errMsg);
   return true;
}

void F3FcPlugin::extensionsInitialized()
{
    /**
     * Create the board objects here.
     *
     */
    F3Fc* f3fc = new F3Fc();
    addAutoReleasedObject(f3fc);
}

void F3FcPlugin::shutdown()
{
}
