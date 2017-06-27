/**
 ******************************************************************************
 * @file       kmlexportplugin.h
 * @see        The GNU Public License (GPL) Version 3
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup KmlExportPlugin
 * @{
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

#ifndef KMLEXPORTPLUGING_
#define KMLEXPORTPLUGING_

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/iconnection.h>
#include <extensionsystem/iplugin.h>
#include "kmlexport.h"

#include <QQueue>
#include <QReadWriteLock>

class KmlExportPlugin;

class KmlExportPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.dronin.plugins.KmlExportPlugin")

public:
    KmlExportPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList &arguments, QString *errorString);
    void shutdown();

private slots:
    void exportToKML();

private:
    Core::Command *exportToKmlCmd;
};
#endif /* KMLEXPORTPLUGING_ */
/**
 * @}
 * @}
 */
