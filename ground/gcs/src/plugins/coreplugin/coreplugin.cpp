/**
 ******************************************************************************
 *
 * @file       coreplugin.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
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
#include "coreplugin.h"
#include "coreimpl.h"
#include "mainwindow.h"
#include <utils/runguard.h>
#include <QtPlugin>
#include <extensionsystem/pluginmanager.h>
#include <QtCore/QtPlugin>
#include <QTimer>

using namespace Core::Internal;

CorePlugin::CorePlugin()
    : m_mainWindow(new MainWindow)
    , m_secondaryAttempts(0)
    , m_secondaryTimer(new QTimer)
{
    connect(m_mainWindow, SIGNAL(splashMessages(QString)), this, SIGNAL(splashMessages(QString)));
    connect(m_mainWindow, SIGNAL(hideSplash()), this, SIGNAL(hideSplash()));
    connect(m_mainWindow, SIGNAL(showSplash()), this, SIGNAL(showSplash()));
}

CorePlugin::~CorePlugin()
{
    m_secondaryTimer->stop();
    delete m_secondaryTimer;
    delete m_mainWindow;
}

bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    if (arguments.contains("crashme")) {
        if ((arguments.length() > 1) && (arguments.at(arguments.indexOf("crashme") + 1) == "yes")) {
            QString *s = 0;
            s->append("crash");
        }
    }
    const bool success = m_mainWindow->init(errorMessage);
    if (success) {
        // nothing right now
    }
    return success;
}

void CorePlugin::extensionsInitialized()
{
    m_mainWindow->extensionsInitialized();

    // check if another app instance has attempted to start periodically
    // and show our instance if so
    m_secondaryTimer->setInterval(1000);
    connect(m_secondaryTimer, &QTimer::timeout, this, [this]() {
        // relying on main() to have already created the instance
        // (with appropriate key)
        quint64 attempts = RunGuard::instance("").secondaryAttempts();
        if (attempts != m_secondaryAttempts && m_mainWindow)
            m_mainWindow->activateWindow();
        m_secondaryAttempts = attempts;
    });
    m_secondaryTimer->start();
}

void CorePlugin::remoteArgument(const QString &arg)
{
    // An empty argument is sent to trigger activation
    // of the window via QtSingleApplication. It should be
    // the last of a sequence.
    if (arg.isEmpty()) {
        m_mainWindow->activateWindow();
    }
}

void CorePlugin::shutdown()
{
    m_mainWindow->shutdown();
}
