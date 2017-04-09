/**
 ******************************************************************************
 *
 * @file       debuggadgetfactory.cpp
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
#include "debuggadgetfactory.h"
#include "debuggadgetwidget.h"
#include "debuggadget.h"
#include <coreplugin/iuavgadget.h>
#include <QTextStream>
#include <QProcessEnvironment>

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    DebugEngine::Level level = DebugEngine::DEBUG;
    switch (type) {
    case QtDebugMsg:
        level = DebugEngine::DEBUG;
        break;
    case QtInfoMsg:
        level = DebugEngine::INFO;
        break;
    case QtWarningMsg:
        level = DebugEngine::WARNING;
        QTextStream(stderr) << "[Warning] " << msg << endl;
        break;
    case QtCriticalMsg:
        level = DebugEngine::CRITICAL;
        QTextStream(stderr) << "[Critical] " << msg << endl;
        break;
    case QtFatalMsg:
        level = DebugEngine::FATAL;
        QTextStream(stderr) << "[FATAL] " << msg << endl;
        break;
    }
    DebugEngine::getInstance()->message(level, msg, QString(context.file), context.line,
                                        QString(context.function));
}

DebugGadgetFactory::DebugGadgetFactory(QObject *parent)
    : IUAVGadgetFactory(QString("DebugGadget"), tr("DebugGadget"), parent)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("NO_DEBUG_GADGET"))
        DebugEngine::getInstance()->message(DebugEngine::INFO,
                                            "Debug gadget disabled by NO_DEBUG_GADGET env. var.");
    else
        qInstallMessageHandler(customMessageHandler);
}

DebugGadgetFactory::~DebugGadgetFactory()
{
}

IUAVGadget *DebugGadgetFactory::createGadget(QWidget *parent)
{
    DebugGadgetWidget *gadgetWidget = new DebugGadgetWidget(parent);

    return new DebugGadget(QString("DebugGadget"), gadgetWidget, parent);
}

/**
 * @}
 * @}
 */
