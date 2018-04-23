/**
 ******************************************************************************
 *
 * @file       debuggadgetwidget.cpp
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
#include "debuggadgetwidget.h"

#include <QDebug>
#include <QStringList>
#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include "debugengine.h"
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QTime>

DebugGadgetWidget::DebugGadgetWidget(QWidget *parent)
    : QLabel(parent)
{
    m_config = new Ui_Form();
    m_config->setupUi(this);
    DebugEngine *de = DebugEngine::getInstance();
    connect(de, &DebugEngine::message, this, &DebugGadgetWidget::message, Qt::QueuedConnection);
    connect(m_config->saveToFile, &QAbstractButton::clicked, this, &DebugGadgetWidget::saveLog);
    connect(m_config->clearLog, &QAbstractButton::clicked, this, &DebugGadgetWidget::clearLog);
}

DebugGadgetWidget::~DebugGadgetWidget()
{
    // Do nothing
}

void DebugGadgetWidget::saveLog()
{
    QString fileName = QFileDialog::getSaveFileName(
            this, tr("Save log File As"),
            QString("gcs-debug-log-%0.html")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")),
            tr("HTML (*.html)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)
        && (file.write(m_config->plainTextEdit->toHtml().toLatin1()) != -1)) {
        file.close();
    } else {
        QMessageBox::critical(this, tr("Log Save"), tr("Unable to save log: ") + fileName,
                              QMessageBox::Ok);
        return;
    }
}

void DebugGadgetWidget::clearLog()
{
    m_config->plainTextEdit->clear();
}

void DebugGadgetWidget::message(DebugEngine::Level level, const QString &msg, const QString &file,
                                const int line, const QString &function)
{
    QColor color;
    QString type;
    switch (level) {
    case DebugEngine::DEBUG:
        color = Qt::blue;
        type = "debug";
        break;
    case DebugEngine::INFO:
        color = Qt::black;
        type = "info";
        break;
    case DebugEngine::WARNING:
        color = Qt::red;
        type = "WARNING";
        break;
    case DebugEngine::CRITICAL:
        color = Qt::red;
        type = "CRITICAL";
        break;
    case DebugEngine::FATAL:
        color = Qt::red;
        type = "FATAL";
        break;
    }

    QString source;
#ifdef QT_DEBUG // only display this extended info to devs
    source = QString("[%0:%1 %2]").arg(file).arg(line).arg(function);
#else
    Q_UNUSED(file);
    Q_UNUSED(line);
    Q_UNUSED(function);
#endif

    m_config->plainTextEdit->setTextColor(color);
    m_config->plainTextEdit->append(
        QString("%0[%1]%2 %3").arg(QTime::currentTime().toString()).arg(type).arg(source).arg(msg));

    QScrollBar *sb = m_config->plainTextEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}

/**
 * @}
 * @}
 */
