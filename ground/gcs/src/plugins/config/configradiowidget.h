/**
 ******************************************************************************
 * @file       configradiowidget.h
 * @brief      Configure the integrated radio
 * @author     dRonin, http://dronin.org Copyright (C) 2018
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
#ifndef CONFIGRADIOWIDGET_H
#define CONFIGRADIOWIDGET_H

#include "ui_integratedradio.h"

#include "uavobjectwidgetutils/configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjects/uavobjectmanager.h"
#include "uavobjects/uavobject.h"

namespace Ui {
class Radio;
}

class ConfigRadioWidget : public ConfigTaskWidget
{
    Q_OBJECT

public:
    ConfigRadioWidget(QWidget *parent = nullptr);
    ~ConfigRadioWidget();

private slots:

private:
    Ui::Radio *ui;

protected:
    void resizeEvent(QResizeEvent *event);
    virtual void enableControls(bool enable);
};

#endif // CONFIGRADIOWIDGET_H
