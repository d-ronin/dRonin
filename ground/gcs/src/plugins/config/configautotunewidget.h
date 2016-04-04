/**
 ******************************************************************************
 *
 * @file       configautotunewidget.h
 * @author     dRonin, http://dronin.org, Copyright (C) 2015
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to adjust or recalculate autotuning
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef CONFIGAUTOTUNE_H
#define CONFIGAUTOTUNE_H

#include "ui_autotune.h"
#include "../uavobjectwidgetutils/configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include "actuatorsettings.h"
#include "stabilizationsettings.h"
#include "systemident.h"
#include <QWidget>
#include <QTimer>
#include <QtNetwork/QNetworkReply>
#include "autotuneshareform.h"
#include "configgadgetwidget.h"

class ConfigAutotuneWidget : public ConfigTaskWidget
{
    Q_OBJECT
public:
    explicit ConfigAutotuneWidget(ConfigGadgetWidget *parent = 0);

private:
    Ui_AutotuneWidget *m_autotune;
    StabilizationSettings::DataFields stabSettings;
    UAVObjectUtilManager* utilMngr;
    AutotuneShareForm *autotuneShareForm;
    ConfigGadgetWidget *parentConfigWidget;

    bool approveSettings(SystemIdent::DataFields systemIdentData);
    QJsonDocument getResultsJson();
    QString getResultsPlainText();
    void saveUserData();
    void loadUserData();

    static const QString databaseUrl;

signals:

public slots:
    void refreshWidgetsValues(UAVObject *obj);
    void updateObjectsFromWidgets();
    void resetSliders();
private slots:
    void recomputeStabilization();
    void saveStabilization();
    void onShareData();
    void onShareToDatabase();
    void onShareToClipboard();
    void onShareToDatabaseComplete(QNetworkReply *reply);
    void onYawTuneToggled(bool checked);
    void onStabSettingsUpdated(UAVObject *obj);
};

#endif // CONFIGAUTOTUNE_H
