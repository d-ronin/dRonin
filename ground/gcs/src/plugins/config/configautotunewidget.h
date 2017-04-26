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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */
#ifndef CONFIGAUTOTUNE_H
#define CONFIGAUTOTUNE_H

#include "ui_autotune.h"
#include "ui_autotuneproperties.h"
#include "ui_autotunesliders.h"
#include "ui_autotunefinalpage.h"
#include "../uavobjectwidgetutils/configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include "actuatorsettings.h"
#include "stabilizationsettings.h"
#include "systemident.h"
#include <QWidget>
#include <QTimer>
#include <QWizardPage>
#include <QtNetwork/QNetworkReply>

#include "configgadgetwidget.h"

struct AutotunedValues
{
    // Inputs
    float damping;
    float noiseSens;

    // Computation status
    bool converged;
    int iterations;

    // Results...
    // -1 means "not calculated"; (don't change)
    float kp[3];
    float ki[3];
    float kd[3];

    float derivativeCutoff;
    float naturalFreq;

    float outerKp;
    float outerKi;
};

class AutotuneMeasuredPropertiesPage : public QWizardPage, private Ui::AutotuneProperties
{
    Q_OBJECT

public:
    explicit AutotuneMeasuredPropertiesPage(QWidget *parent,
                                            SystemIdent::DataFields &systemIdentData);
    void initializePage();

private:
    SystemIdent::DataFields sysIdent;
};

class AutotuneSlidersPage : public QWizardPage, private Ui::AutotuneSliders
{
    Q_OBJECT

public:
    explicit AutotuneSlidersPage(QWidget *parent, SystemIdent::DataFields &systemIdentData,
                                 struct AutotunedValues *autoValues);

    bool isComplete() const;

private:
    SystemIdent::DataFields sysIdent;
    AutotunedValues *av;

    void setText(QLabel *lbl, double value, int precision);

private slots:
    void compute();
    void resetSliders();
};

class AutotuneFinalPage : public QWizardPage, public Ui::AutotuneFinalPage
{
    Q_OBJECT

public:
    explicit AutotuneFinalPage(QWidget *parent);
};

class ConfigAutotuneWidget : public ConfigTaskWidget
{
    Q_OBJECT
public:
    explicit ConfigAutotuneWidget(ConfigGadgetWidget *parent = 0);

private:
    Ui_AutotuneWidget *m_autotune;
    UAVObjectUtilManager *utilMngr;
    ConfigGadgetWidget *parentConfigWidget;
    static const QString databaseUrl;

    QString systemIdentValid(SystemIdent::DataFields &data, bool *okToContinue);

    QJsonDocument getResultsJson(AutotuneFinalPage *autotuneShareForm, struct AutotunedValues *av);

    void stuffShareForm(AutotuneFinalPage *autotuneShareForm);
    void persistShareForm(AutotuneFinalPage *autotuneShareForm);
    void checkNewAutotune();

private slots:
    void openAutotuneDialog();
    void openAutotuneDialog(bool autoOpened);

    void openAutotuneFile();

    void atConnected();
    void atDisconnected();
};

#endif // CONFIGAUTOTUNE_H
