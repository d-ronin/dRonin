/**
 ******************************************************************************
 *
 * @file       debugengine.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup DebugGadgetPlugin Debug Gadget Plugin
 * @{
 * @brief A place holder gadget plugin
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

#ifndef DEBUGENGINE_H
#define DEBUGENGINE_H
#include <QTextBrowser>
#include <QPointer>
#include <QObject>
#include "debuggadget_global.h"

class DEBUGGADGET_EXPORT DebugEngine : public QObject
{
    Q_OBJECT
    // Add all missing constructor etc... to have singleton
    DebugEngine();

public:
    enum Level {
        DEBUG,
        INFO,
        WARNING,
        CRITICAL,
        FATAL,
    };

    static DebugEngine *getInstance();

signals:
    void message(DebugEngine::Level level, const QString &msg, const QString &file = "",
                 const int line = 0, const QString &function = "");
};

#endif // DEBUGENGINE_H

/**
 * @}
 * @}
 */
