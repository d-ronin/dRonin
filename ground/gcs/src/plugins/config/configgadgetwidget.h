/**
 ******************************************************************************
 *
 * @file       configgadgetwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to update settings in the firmware
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
#ifndef CONFIGGADGETWIDGET_H
#define CONFIGGADGETWIDGET_H

#include "uavtalk/telemetrymanager.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjects/uavobjectmanager.h"
#include "uavobjects/uavobject.h"
#include "objectpersistence.h"
#include <QWidget>
#include <QList>
#include <QTextBrowser>
#include "utils/pathutils.h"
#include "utils/mytabbedstackwidget.h"
#include "../uavobjectwidgetutils/configtaskwidget.h"

class ConfigGadgetWidget : public QWidget
{
    Q_OBJECT
    QTextBrowser *help;
    int chunk;

public:
    ConfigGadgetWidget(QWidget *parent = nullptr);
    ~ConfigGadgetWidget();
    enum widgetTabs {
        hardware = 0,
        aircraft,
        input,
        output,
        sensors,
        stabilization,
        modules,
        txpid,
        autotune,
        osd,
        radio
    };
    void startInputWizard();
    void changeTab(int i);

public slots:
    void onAutopilotConnect();
    void onAutopilotDisconnect();
    void tabAboutToChange(int i, bool *);
    void deferredLoader();

signals:
    void autopilotConnected();
    void autopilotDisconnected();

protected:
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    MyTabbedStackWidget *ftw;

private:
    UAVDataObject *oplinkStatusObj;
    int lastTabIndex;
    // A timer that timesout the connction to the OPLink.
    QTimer *oplinkTimeout;
    bool oplinkConnected;
};

#endif // CONFIGGADGETWIDGET_H
