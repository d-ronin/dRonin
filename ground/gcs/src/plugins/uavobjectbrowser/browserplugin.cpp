/**
 ******************************************************************************
 *
 * @file       mapplugin.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVObjectBrowserPlugin UAVObject Browser Plugin
 * @{
 * @brief The UAVObject Browser gadget plugin
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
#include "browserplugin.h"
#include "uavobjectbrowserfactory.h"
#include <QtPlugin>
#include <QStringList>
#include <extensionsystem/pluginmanager.h>

BrowserPlugin::BrowserPlugin()
{
   // Do nothing
}

BrowserPlugin::~BrowserPlugin()
{
   // Do nothing
}

bool BrowserPlugin::initialize(const QStringList& args, QString *errMsg)
{
   Q_UNUSED(args);
   Q_UNUSED(errMsg);
   mf = new UAVObjectBrowserFactory(this);
   addAutoReleasedObject(mf);

   return true;
}

void BrowserPlugin::extensionsInitialized()
{
   // Do nothing
}

void BrowserPlugin::shutdown()
{
   // Do nothing
}
