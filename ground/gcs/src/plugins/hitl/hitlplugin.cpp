/**
 ******************************************************************************
 *
 * @file       hitlplugin.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup HITLPlugin HITL Plugin
 * @{
 * @brief The Hardware In The Loop plugin
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
#include "hitlplugin.h"
#include "hitlfactory.h"
#include <QtPlugin>
#include <QStringList>
#include <extensionsystem/pluginmanager.h>
#include "fgsimulator.h"
#include "xplanesimulator.h"

QList<SimulatorCreator* > HITLPlugin::typeSimulators;

HITLPlugin::HITLPlugin()
{
   // Do nothing
}

HITLPlugin::~HITLPlugin()
{
   // Do nothing
}

bool HITLPlugin::initialize(const QStringList& args, QString *errMsg)
{
   Q_UNUSED(args);
   Q_UNUSED(errMsg);
   mf = new HITLFactory(this);

   addAutoReleasedObject(mf);

   addSimulator(new FGSimulatorCreator("FG","FlightGear"));
   addSimulator(new XplaneSimulatorCreator("X-Plane","X-Plane"));

   return true;
}

void HITLPlugin::extensionsInitialized()
{
   // Do nothing
}

void HITLPlugin::shutdown()
{
   // Do nothing
}

