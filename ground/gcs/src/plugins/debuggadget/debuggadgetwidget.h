/**
 ******************************************************************************
 *
 * @file       debuggadgetwidget.h
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

#ifndef DEBUGGADGETWIDGET_H_
#define DEBUGGADGETWIDGET_H_

#include <QLabel>
#include "ui_debug.h"
#include "debugengine.h"
class DebugGadgetWidget : public QLabel
{
    Q_OBJECT

public:
    DebugGadgetWidget(QWidget *parent = nullptr);
    ~DebugGadgetWidget();

private:
    Ui_Form *m_config;
private slots:
    void saveLog();
    void clearLog();
    void message(DebugEngine::Level level, const QString &msg, const QString &file, const int line,
                 const QString &function);
};
#endif /* DEBUGGADGETWIDGET_H_ */

/**
 * @}
 * @}
 */
