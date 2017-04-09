/**
 ******************************************************************************
 *
 * @file       coreplugin.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup CorePlugin Core Plugin
 * @{
 * @brief The Core GCS plugin
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

#ifndef COREPLUGIN_H
#define COREPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Core {
namespace Internal {

    class MainWindow;

    class CorePlugin : public ExtensionSystem::IPlugin
    {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.dronin.plugins.Core")

    public:
        CorePlugin();
        ~CorePlugin();

        virtual bool initialize(const QStringList &arguments, QString *errorMessage = 0);
        virtual void extensionsInitialized();
        virtual void shutdown();

    public slots:
        void remoteArgument(const QString & = QString());
    signals:
        void splashMessages(QString);
        void hideSplash();
        void showSplash();

    private:
        MainWindow *m_mainWindow;
    };

} // namespace Internal
} // namespace Core

#endif // COREPLUGIN_H
