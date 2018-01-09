/**
 ******************************************************************************
 * @file       configosdwidget.h
 * @brief      Configure the OSD
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
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
#ifndef CONFIGOSDWIDGET_H
#define CONFIGOSDWIDGET_H

#include "ui_osd.h"

#include "uavobjectwidgetutils/configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjects/uavobjectmanager.h"
#include "uavobjects/uavobject.h"

#include "onscreendisplaysettings.h"
#include "manualcontrolcommand.h"
#include "manualcontrolsettings.h"

#include "onscreendisplaypagesettings.h"
#include "onscreendisplaypagesettings2.h"
#include "onscreendisplaypagesettings3.h"
#include "onscreendisplaypagesettings4.h"

namespace Ui {
class Osd;
class OsdPage;
}

class ConfigOsdWidget : public ConfigTaskWidget
{
    Q_OBJECT

public:
    ConfigOsdWidget(QWidget *parent = nullptr);
    ~ConfigOsdWidget();

private slots:
    void movePageSlider();
    void updatePositionSlider();
    void setCustomText();
    void getCustomText();

private:
    void setupOsdPage(Ui::OsdPage *page, QWidget *page_widget, const QString &objName);
    void copyOsdPage(int to, int from);
    quint8 scaleSwitchChannel(quint8 channelNumber, quint8 switchPositions);

    Ui::Osd *ui;
    Ui::OsdPage *ui_pages[4];
    QWidget *pages[4];

    OnScreenDisplaySettings *osdSettingsObj;

    ManualControlSettings *manualSettingsObj;
    ManualControlSettings::DataFields manualSettingsData;
    ManualControlCommand *manualCommandObj;
    ManualControlCommand::DataFields manualCommandData;

protected:
    void resizeEvent(QResizeEvent *event);
    virtual void enableControls(bool enable);
};

#endif // CONFIGOSDWIDGET_H
