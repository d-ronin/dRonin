/**
 ******************************************************************************
 *
 * @file       nazeplugin.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_Naze Naze boards support Plugin
 * @{
 * @brief Plugin to support Naze boards
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

#include "nazeplugin.h"
#include "naze.h"
#include <QtPlugin>


NazePlugin::NazePlugin()
{
   // Do nothing
}

NazePlugin::~NazePlugin()
{
   // Do nothing
}

bool NazePlugin::initialize(const QStringList& args, QString *errMsg)
{
   Q_UNUSED(args);
   Q_UNUSED(errMsg);
   return true;
}

void NazePlugin::extensionsInitialized()
{
    /**
     * Create the board objects here.
     *
     */
    Naze* naze = new Naze();
    addAutoReleasedObject(naze);

}

void NazePlugin::shutdown()
{
}
