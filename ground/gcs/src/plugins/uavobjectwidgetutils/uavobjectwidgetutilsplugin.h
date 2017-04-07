/**
 ******************************************************************************
 *
 * @file       uavobjectwidgetutilsplugin.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVObjectWidgetUtils Plugin
 * @{
 * @brief Utility plugin for UAVObject to Widget relation management
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
#ifndef UAVOBJECTWIDGETUTILSPLUGIN_H
#define UAVOBJECTWIDGETUTILSPLUGIN_H

#include "uavobjectwidgetutils_global.h"
#include <QtPlugin>
#include <extensionsystem/iplugin.h>

class UAVOBJECTWIDGETUTILS_EXPORT UAVObjectWidgetUtilsPlugin
        : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.dronin.plugins.UAVObjectWidgetUtils")

public:
    UAVObjectWidgetUtilsPlugin();
    ~UAVObjectWidgetUtilsPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList &arguments, QString *errorString);
    void shutdown();
};

#endif
