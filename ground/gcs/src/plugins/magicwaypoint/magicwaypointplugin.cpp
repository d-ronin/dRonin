/**
 ******************************************************************************
 *
 * @file       GCSControlplugin.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup GCSControlGadgetPlugin GCSControl Gadget Plugin
 * @{
 * @brief A gadget to control the UAV, either from the keyboard or a joystick
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
#include "magicwaypointplugin.h"
#include "magicwaypointgadgetfactory.h"
#include <QDebug>
#include <QtPlugin>
#include <QStringList>
#include <extensionsystem/pluginmanager.h>

MagicWaypointPlugin::MagicWaypointPlugin()
{
    // Do nothing
}

MagicWaypointPlugin::~MagicWaypointPlugin()
{
    // Do nothing
}

bool MagicWaypointPlugin::initialize(const QStringList &args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);
    mf = new MagicWaypointGadgetFactory(this);
    addAutoReleasedObject(mf);

    return true;
}

void MagicWaypointPlugin::extensionsInitialized()
{
    // Do nothing
}

void MagicWaypointPlugin::shutdown()
{
    // Do nothing
}

/**
  * @}
  * @}
  */
