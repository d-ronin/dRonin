/**
 ******************************************************************************
 *
 * @file       donothingplugin.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup DoNothingPlugin Do Nothing Plugin
 * @{
 * @brief A minimal example plugin
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
#include "donothingplugin.h"
#include <QtPlugin>
#include <QStringList>

DoNothingPlugin::DoNothingPlugin()
{
    // Do nothing
}

DoNothingPlugin::~DoNothingPlugin()
{
    // Do nothing
}

bool DoNothingPlugin::initialize(const QStringList &args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    return true;
}

void DoNothingPlugin::extensionsInitialized()
{
    // Do nothing
}

void DoNothingPlugin::shutdown()
{
    // Do nothing
}
Q_EXPORT_PLUGIN(DoNothingPlugin)
