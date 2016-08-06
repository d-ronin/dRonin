/**
 ******************************************************************************
 * @file       droninplugin.cpp
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_dRonin dRonin board support plugin
 * @{
 * @brief Supports dRonin board configuration
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "droninplugin.h"
#include "simulation.h"
#include <QtPlugin>


DroninPlugin::DroninPlugin()
{
}

DroninPlugin::~DroninPlugin()
{
}

bool DroninPlugin::initialize(const QStringList &arguments, QString *errorString)
{
   Q_UNUSED(arguments);
   Q_UNUSED(errorString);
   return true;
}

void DroninPlugin::extensionsInitialized()
{
    // Init boards
    Simulation* sim = new Simulation();
    addAutoReleasedObject(sim);

}

void DroninPlugin::shutdown()
{
}

/**
 * @}
 * @}
 */
