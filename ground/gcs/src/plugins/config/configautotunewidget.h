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

#include "../uavobjectwidgetutils/configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjects/uavobjectmanager.h"
#include "uavobjects/uavobject.h"
#include "actuatorsettings.h"
#include "stabilizationsettings.h"
#include "systemident.h"

#include <QChart>
#include <QLineSeries>
#include <QTimer>
#include <QWidget>
#include <QWizardPage>
#include <QtNetwork/QNetworkReply>

QT_CHARTS_USE_NAMESPACE

#include "ui_autotune.h"
#include "ui_autotunebeginning.h"
#include "ui_autotuneproperties.h"
#include "ui_autotunesliders.h"
#include "ui_autotunefinalpage.h"
#include "configgadgetwidget.h"

struct AutotunedValues
{
    bool valid;

    QByteArray *data;

    // Parameters
    float tau[3];
    float beta[3];
    float bias[3];
    float noise[3];

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

    QLineSeries *model[3];
    QLineSeries *actual[3];
};

class AutotuneBeginningPage : public QWizardPage, private Ui::AutotuneBeginning
{
    Q_OBJECT

public:
    explicit AutotuneBeginningPage(QWidget *parent, bool autoOpened,
                                            AutotunedValues *autoValues);

    void initializePage();

    bool isComplete() const;

private:
    QString tuneValid(bool *okToContinue) const;

    AutotunedValues *av;
    bool autoOpened;
    bool dataValid;

    /* Need a better place for all of these implementation things.  But
     * for now this is at least encapsulated and won't get tainted
     * elsewhere
     */
    const uint64_t ATFLASH_MAGIC = 0x656e755480008041;

    struct at_flash_header
    {
        uint64_t magic;
        uint16_t wiggle_points;
        uint16_t aux_data_len;
        uint16_t sample_rate;

        // Consider total number of averages here
        uint16_t resv;
    };

    struct at_measurement
    {
        float y[3]; /* Gyro measurements */
        float u[3]; /* Actuator desired */
    };

    struct at_flash
    {
        struct at_flash_header hdr;

        struct at_measurement data[];
    };

    bool processAutotuneData();
    void biquadFilter(float cutoff, int pts, QVector<float> &data);
    float getSampleDelay(int pts, const QVector<float> &delayed,
            const QVector<float> &orig);

private slots:
    void doDownloadAndProcess();

};

class AutotuneMeasuredPropertiesPage : public QWizardPage, private Ui::AutotuneProperties
{
    Q_OBJECT

public:
    explicit AutotuneMeasuredPropertiesPage(QWidget *parent,
                                            AutotunedValues *autoValues);
    void initializePage();

private:
    AutotunedValues *av;
    QChart *makeChart(int axis);
};

class AutotuneSlidersPage : public QWizardPage, private Ui::AutotuneSliders
{
    Q_OBJECT

public:
    explicit AutotuneSlidersPage(QWidget *parent,
                                 AutotunedValues *autoValues);

    bool isComplete() const;

    void initializePage();

private:
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

    QString tuneValid(AutotunedValues &data, bool *okToContinue) const;

    QJsonDocument getResultsJson(AutotuneFinalPage *autotuneShareForm, AutotunedValues *av);

    void stuffShareForm(AutotuneFinalPage *autotuneShareForm);
    void persistShareForm(AutotuneFinalPage *autotuneShareForm);
    void checkNewAutotune();

private slots:
    void openAutotuneDialog();
    void openAutotuneDialog(bool autoOpened, AutotunedValues *precalc_vals = nullptr);

    void openAutotuneFile();

    void atConnected();
    void atDisconnected();
};

#endif // CONFIGAUTOTUNE_H
