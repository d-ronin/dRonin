/**
******************************************************************************
* @file       aeroquadplugin.cpp
* @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
*
* @addtogroup GCSPlugins GCS Plugins
* @{
* @addtogroup Boards_AeroQuadPlugin AeroQuad boards support Plugin
* @{
* @brief Plugin to support AeroQuad boards
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

#ifndef AEROQUADPLUGIN_H
#define AEROQUADPLUGIN_H

#include <extensionsystem/iplugin.h>

class AeroQuadPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.dronin.plugins.AeroQuad")

public:
    AeroQuadPlugin();
    ~AeroQuadPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList &arguments, QString *errorString);
    void shutdown();
};

#endif // AEROQUADPLUGIN_H
