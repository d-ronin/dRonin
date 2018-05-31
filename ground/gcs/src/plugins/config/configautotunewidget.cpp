/**
 ******************************************************************************
 *
 * @file       configautotunewidget.cpp
 * @author     dRonin, http://dronin.org, Copyright (C) 2015-2016
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

#define _USE_MATH_DEFINES

#include <cmath>
#include <algorithm>
#include <numeric>

#include "ffft/FFTReal.h"

#include "configautotunewidget.h"

#include <uavobjectutil/devicedescriptorstruct.h>
#include <uavobjectutil/uavobjectutilmanager.h>

#include "altitudeholdsettings.h"
#include "manualcontrolsettings.h"
#include "mixersettings.h"
#include "modulesettings.h"
#include "sensorsettings.h"
#include "stabilizationsettings.h"
#include "systemident.h"
#include "systemsettings.h"
#include "coreplugin/generalsettings.h"
#include "extensionsystem/pluginmanager.h"
#include "coreplugin/iboardtype.h"

#include "physical_constants.h"

#include <QtAlgorithms>
#include <QChartView>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QList>
#include <QPushButton>
#include <QStringList>
#include <QTextEdit>
#include <QUrl>
#include <QValueAxis>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <QWizard>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

//#define CONF_ATUNE_DEBUG
#ifdef CONF_ATUNE_DEBUG
#define CONF_ATUNE_QXTLOG_DEBUG(...) qDebug() << __VA_ARGS__
#else // CONF_ATUNE_DEBUG
#define CONF_ATUNE_QXTLOG_DEBUG(...)
#endif // CONF_ATUNE_DEBUG

const QString ConfigAutotuneWidget::databaseUrl =
    QString("http://dronin-autotown.appspot.com/storeTune");

ConfigAutotuneWidget::ConfigAutotuneWidget(ConfigGadgetWidget *parent)
    : ConfigTaskWidget(parent)
{
    parentConfigWidget = parent;
    m_autotune = new Ui_AutotuneWidget();
    m_autotune->setupUi(this);

    // Connect automatic signals
    autoLoadWidgets();
    disableMouseWheelEvents();

    autoLoadWidgets();

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    utilMngr = pm->getObject<UAVObjectUtilManager>();

    connect(this, &ConfigTaskWidget::autoPilotConnected, this, &ConfigAutotuneWidget::atConnected,
            Qt::QueuedConnection);
    connect(this, &ConfigTaskWidget::autoPilotDisconnected, this,
            &ConfigAutotuneWidget::atDisconnected, Qt::QueuedConnection);
    connect(m_autotune->adjustTune, &QPushButton::pressed, this,
            QOverload<>::of(&ConfigAutotuneWidget::openAutotuneDialog));
    connect(m_autotune->fromDataFileBtn, &QPushButton::pressed, this,
            &ConfigAutotuneWidget::openAutotuneFile);

    m_autotune->adjustTune->setEnabled(isAutopilotConnected());
}

void ConfigAutotuneWidget::atConnected()
{
    m_autotune->adjustTune->setEnabled(true);
    checkNewAutotune();
}

void ConfigAutotuneWidget::atDisconnected()
{
    m_autotune->adjustTune->setEnabled(false);
}

void ConfigAutotuneWidget::checkNewAutotune()
{
    SystemIdent *systemIdent = SystemIdent::GetInstance(getObjectManager());

    SystemIdent::DataFields systemIdentData = systemIdent->getData();

    if (systemIdentData.NewTune) {
        // There's a new tune!  Let's talk to the user about it.

        // First though, cue resetting this flag to false.
        systemIdentData.NewTune = false;

        systemIdent->setData(systemIdentData);

        systemIdent->updated();

        // And persist it, so we're good for next time.
        saveObjectToSD(systemIdent);

        // Now let's get that dialog open.
        openAutotuneDialog(true);
    }
}

/**
 * @brief ConfigAutotuneWidget::generateResultsJson
 * @return QJsonDocument containing autotune result data
 */
QJsonDocument ConfigAutotuneWidget::getResultsJson(AutotuneFinalPage *autotuneShareForm,
                                                   AutotunedValues *tuneState)
{
    deviceDescriptorStruct firmware;
    utilMngr->getBoardDescriptionStruct(firmware);

    QJsonObject rawSettings;

    QJsonObject json;
    json["dataVersion"] = 3;
    json["uniqueId"] =
        QString(QCryptographicHash::hash(utilMngr->getBoardCPUSerial(), QCryptographicHash::Sha256)
                    .toHex());

    QJsonObject vehicle, fw;
    Core::IBoardType *board = utilMngr->getBoardType();
    if (board) {
        fw["board"] = board->shortName();
    }

    fw["tag"] = firmware.gitTag;
    fw["commit"] = firmware.gitHash;
    QDateTime fwDate = QDateTime::fromString(firmware.gitDate, "yyyyMMdd hh:mm");
    fwDate.setTimeSpec(Qt::UTC); // this makes it append a Z to the string indicating UTC
    fw["date"] = fwDate.toString(Qt::ISODate);
    vehicle["firmware"] = fw;

    SystemSettings *sysSettings = SystemSettings::GetInstance(getObjectManager());

    rawSettings[sysSettings->getName()] = sysSettings->getJsonRepresentation();

    ActuatorSettings *actSettings = ActuatorSettings::GetInstance(getObjectManager());
    rawSettings[actSettings->getName()] = actSettings->getJsonRepresentation();

    StabilizationSettings *stabSettings = StabilizationSettings::GetInstance(getObjectManager());
    rawSettings[stabSettings->getName()] = stabSettings->getJsonRepresentation();

    SystemIdent *systemIdent = SystemIdent::GetInstance(getObjectManager());
    rawSettings[systemIdent->getName()] = systemIdent->getJsonRepresentation();

    SensorSettings *senSettings = SensorSettings::GetInstance(getObjectManager());
    rawSettings[senSettings->getName()] = senSettings->getJsonRepresentation();

    ManualControlSettings *manSettings = ManualControlSettings::GetInstance(getObjectManager());
    rawSettings[manSettings->getName()] = manSettings->getJsonRepresentation();

    MixerSettings *mixSettings = MixerSettings::GetInstance(getObjectManager());
    rawSettings[mixSettings->getName()] = mixSettings->getJsonRepresentation();

    // Query the board plugin for the connected board to get the specific
    // hw settings object
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    if (pm != NULL) {
        UAVObjectUtilManager *uavoUtilManager = pm->getObject<UAVObjectUtilManager>();
        Core::IBoardType *board = uavoUtilManager->getBoardType();
        if (board != NULL) {
            QString hwSettingsName = board->getHwUAVO();

            UAVObject *hwSettings = getObjectManager()->getObject(hwSettingsName);
            rawSettings[hwSettings->getName()] = hwSettings->getJsonRepresentation();
        }
    }

    vehicle["type"] = autotuneShareForm->acType->currentText();
    vehicle["size"] = autotuneShareForm->acVehicleSize->text();
    vehicle["weight"] = autotuneShareForm->acWeight->text();
    vehicle["batteryCells"] = autotuneShareForm->acBatteryCells->currentText();
    vehicle["esc"] = autotuneShareForm->acEscs->text();
    vehicle["motor"] = autotuneShareForm->acMotors->text();
    vehicle["props"] = autotuneShareForm->acProps->text();
    json["vehicle"] = vehicle;

    json["userObservations"] = autotuneShareForm->teObservations->toPlainText();

    QJsonObject identification;
    QJsonObject roll_ident;
    roll_ident["gain"] = tuneState->beta[0];
    roll_ident["bias"] = tuneState->bias[0];
    roll_ident["noise"] = tuneState->noise[0];
    roll_ident["tau"] = tuneState->tau[0];
    identification["roll"] = roll_ident;

    QJsonObject pitch_ident;
    pitch_ident["gain"] = tuneState->beta[1];
    pitch_ident["bias"] = tuneState->bias[1];
    pitch_ident["noise"] = tuneState->noise[1];
    pitch_ident["tau"] = tuneState->tau[1];
    identification["pitch"] = pitch_ident;

    QJsonObject yaw_ident;
    yaw_ident["gain"] = tuneState->beta[2];
    yaw_ident["bias"] = tuneState->bias[2];
    yaw_ident["noise"] = tuneState->noise[2];
    yaw_ident["tau"] = tuneState->tau[2];
    identification["yaw"] = yaw_ident;

    identification["tau"] = tuneState->tau[0];
    json["identification"] = identification;

    QJsonObject tuning, parameters, computed, misc;
    parameters["damping"] = tuneState->damping;
    parameters["noiseSensitivity"] = tuneState->noiseSens;

    tuning["parameters"] = parameters;
    computed["naturalFrequency"] = tuneState->naturalFreq;
    computed["derivativeCutoff"] = tuneState->derivativeCutoff;
    computed["converged"] = tuneState->converged;
    computed["iterations"] = tuneState->iterations;

    QJsonObject gains;
    QJsonObject roll_gain, pitch_gain, yaw_gain, outer_gain, vert_gain;
    roll_gain["kp"] = tuneState->kp[0];
    roll_gain["ki"] = tuneState->ki[0];
    roll_gain["kd"] = tuneState->kd[0];
    gains["roll"] = roll_gain;
    pitch_gain["kp"] = tuneState->kp[1];
    pitch_gain["ki"] = tuneState->ki[1];
    pitch_gain["kd"] = tuneState->kd[1];
    gains["pitch"] = pitch_gain;
    yaw_gain["kp"] = tuneState->kp[2];
    yaw_gain["ki"] = tuneState->ki[2];
    yaw_gain["kd"] = tuneState->kd[2];
    gains["yaw"] = yaw_gain;
    outer_gain["kp"] = tuneState->outerKp;
    outer_gain["ki"] = tuneState->outerKi;
    gains["outer"] = outer_gain;
    vert_gain["kp"] = tuneState->vertSpeedKp;
    vert_gain["ki"] = tuneState->vertSpeedKi;
    vert_gain["poskp"] = tuneState->vertPosKp;
    computed["gains"] = gains;
    tuning["computed"] = computed;
    json["tuning"] = tuning;

    json["rawSettings"] = rawSettings;

    QByteArray compressedData = qCompress(tuneState->data, 9);
    json["rawTuneData"] = QString(compressedData.toBase64());

    return QJsonDocument(json);
}

void ConfigAutotuneWidget::persistShareForm(AutotuneFinalPage *autotuneShareForm)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Core::Internal::GeneralSettings *settings = pm->getObject<Core::Internal::GeneralSettings>();
    settings->setObservations(autotuneShareForm->teObservations->toPlainText());
    settings->setBoardType(autotuneShareForm->acBoard->text());
    settings->setMotors(autotuneShareForm->acMotors->text());
    settings->setESCs(autotuneShareForm->acEscs->text());
    settings->setProps(autotuneShareForm->acProps->text());
    settings->setWeight(autotuneShareForm->acWeight->text().toInt());
    settings->setVehicleSize(autotuneShareForm->acVehicleSize->text().toInt());
    settings->setVehicleType(autotuneShareForm->acType->currentText());
    settings->setBatteryCells(autotuneShareForm->acBatteryCells->currentText().toInt());
}

void ConfigAutotuneWidget::stuffShareForm(AutotuneFinalPage *autotuneShareForm)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Core::Internal::GeneralSettings *settings = pm->getObject<Core::Internal::GeneralSettings>();

    autotuneShareForm->teObservations->setText(settings->getObservations());
    autotuneShareForm->acBoard->setText(settings->getBoardType());
    autotuneShareForm->acMotors->setText(settings->getMotors());
    autotuneShareForm->acEscs->setText(settings->getESCs());
    autotuneShareForm->acProps->setText(settings->getProps());
    autotuneShareForm->acWeight->setText(QString::number(settings->getWeight()));
    autotuneShareForm->acVehicleSize->setText(QString::number(settings->getVehicleSize()));
    autotuneShareForm->acType->setCurrentText(settings->getVehicleType());
    autotuneShareForm->acBatteryCells->setCurrentText(QString::number(settings->getBatteryCells()));

    SystemSettings *sysSettings = SystemSettings::GetInstance(getObjectManager());
    if (sysSettings) {
        UAVObjectField *frameType = sysSettings->getField("AirframeType");
        if (frameType) {
            autotuneShareForm->acType->clear();
            autotuneShareForm->acType->addItems(frameType->getOptions());

            QString currentFrameType;
            currentFrameType = frameType->getValue().toString();
            if (!currentFrameType.isNull()) {
                autotuneShareForm->acType->setEditable(false);
                autotuneShareForm->acType->setCurrentText(currentFrameType);
            }
        }
    }

    Core::IBoardType *board = utilMngr->getBoardType();

    if (board) {
        autotuneShareForm->acBoard->setText(board->shortName());
        autotuneShareForm->acBoard->setEnabled(false);
    }
}

void ConfigAutotuneWidget::openAutotuneFile()
{
    QString fileName =
        QFileDialog::getOpenFileName(this, tr("Open autotune partition"), "",
                                     tr("Partition image Files (*.bin) ;; All files (*.*)"));

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        /* XXX error dialog */
        return;
    }

    AutotunedValues vals = { };
    vals.data = file.readAll();

    openAutotuneDialog(false, &vals);
}

void ConfigAutotuneWidget::openAutotuneDialog()
{
    openAutotuneDialog(false);
}

void ConfigAutotuneWidget::openAutotuneDialog(bool autoOpened,
        AutotunedValues *precalc_vals)
{
    QWizard wizard(NULL, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint
                       | Qt::WindowCloseButtonHint);

    wizard.setPixmap(QWizard::BackgroundPixmap, QPixmap(":/configgadget/images/autotunebg.png"));

    // Used to track state across the entire wizard path.
    AutotunedValues av = {};

    if (precalc_vals) {
        av = *precalc_vals;
    }

    av.converged = false;

    // The first page is simple; construct it from scratch.
    QWizardPage *beginning = new AutotuneBeginningPage(NULL, autoOpened, &av);

    // Keep a cancel button, even on OS X.
    wizard.setOption(QWizard::NoCancelButton, false);

    wizard.setMinimumSize(735, 600);

    wizard.addPage(beginning);

    AutotuneFinalPage *pg = new AutotuneFinalPage(NULL);

    wizard.addPage(new AutotuneMeasuredPropertiesPage(NULL, &av));
    wizard.addPage(new AutotuneSlidersPage(NULL, &av));

    stuffShareForm(pg);

    wizard.addPage(pg);

    wizard.setWindowTitle("Autotune Wizard");
    wizard.exec();

    if (av.valid && (wizard.result() == QDialog::Accepted) && av.converged) {
        // Apply and save data to board.
        StabilizationSettings *stabilizationSettings =
            StabilizationSettings::GetInstance(getObjectManager());
        Q_ASSERT(stabilizationSettings);
        if (!stabilizationSettings) {
            qWarning() << "Abandoning autotune commit because no StabSettings";
            return;
        }

        StabilizationSettings::DataFields stabData = stabilizationSettings->getData();

        stabData.RollRatePID[StabilizationSettings::ROLLRATEPID_KP] = av.kp[0];
        stabData.RollRatePID[StabilizationSettings::ROLLRATEPID_KI] = av.ki[0];
        stabData.RollRatePID[StabilizationSettings::ROLLRATEPID_KD] = av.kd[0];
        stabData.PitchRatePID[StabilizationSettings::PITCHRATEPID_KP] = av.kp[1];
        stabData.PitchRatePID[StabilizationSettings::PITCHRATEPID_KI] = av.ki[1];
        stabData.PitchRatePID[StabilizationSettings::PITCHRATEPID_KD] = av.kd[1];
        if (av.kp[2] > 0) {
            stabData.YawRatePID[StabilizationSettings::YAWRATEPID_KP] = av.kp[2];
            stabData.YawRatePID[StabilizationSettings::YAWRATEPID_KI] = av.ki[2];
            stabData.YawRatePID[StabilizationSettings::YAWRATEPID_KD] = av.kd[2];
        }

        stabData.DerivativeCutoff = av.derivativeCutoff;

        stabData.RollPI[StabilizationSettings::ROLLPI_KP] = av.outerKp;
        stabData.RollPI[StabilizationSettings::ROLLPI_KI] = av.outerKi;
        stabData.PitchPI[StabilizationSettings::PITCHPI_KP] = av.outerKp;
        stabData.PitchPI[StabilizationSettings::PITCHPI_KI] = av.outerKi;

        stabilizationSettings->setData(stabData);
        stabilizationSettings->updated();

        saveObjectToSD(stabilizationSettings);


        SystemIdent *systemIdent = SystemIdent::GetInstance(getObjectManager());
        if (systemIdent) {
            SystemIdent::DataFields systemIdentData = systemIdent->getData();

            for (int i = 0; i < 3; i++) {
                systemIdentData.Tau[i] = av.tau[i];
                systemIdentData.Beta[i] = av.beta[i];
            }

            systemIdent->setData(systemIdentData);
            systemIdent->updated();

            saveObjectToSD(systemIdent);
        }

        if (av.vertSpeedKp > 0) {
            AltitudeHoldSettings *altSettings =
                AltitudeHoldSettings::GetInstance(getObjectManager());

            if (!altSettings) {
                qWarning() << "Not committing alt hold data because obj not present";
            } else {
                altSettings->setPositionKp(av.vertPosKp);
                altSettings->setVelocityKp(av.vertSpeedKp);
                altSettings->setVelocityKi(av.vertSpeedKi);
                altSettings->updated();
                saveObjectToSD(altSettings);
            }
        }

        if (pg->shareBox->isChecked()) {
            persistShareForm(pg);

            // share to autotown

            QJsonDocument json = getResultsJson(pg, &av);

            // Do this in the background, completely async.
            QUrl url(databaseUrl);
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader,
                              "application/json; charset=utf-8");
            QNetworkAccessManager *manager = new QNetworkAccessManager(this);
            QNetworkReply *reply = manager->post(request, json.toJson());
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        }
    }
}

AutotuneMeasuredPropertiesPage::AutotuneMeasuredPropertiesPage(QWidget *parent,
                                                               AutotunedValues *autoValues)
    : QWizardPage(parent)
{
    setupUi(this);

    this->tuneState = autoValues;
}

QChart *AutotuneMeasuredPropertiesPage::makeChart(int axis) {
    QChart *chart;

    chart = new QChart();

    tuneState->actual[axis]->setName(tr("Actual"));
    chart->addSeries(tuneState->actual[axis]);
    tuneState->model[axis]->setName(tr("Predicted"));
    chart->addSeries(tuneState->model[axis]);

    chart->createDefaultAxes();

    chart->axisX()->setTitleText(tr("Time"));
    dynamic_cast<QValueAxis *>(chart->axisX())->setLabelFormat("%.0f ms");
    chart->axisY()->setTitleText(tr("Angular acceleration"));
    chart->axisY()->setLabelsVisible(false);

    return chart;
}

void AutotuneMeasuredPropertiesPage::initializePage()
{
    measuredRollGain->setText(QString::number(tuneState->beta[0], 'f', 2));
    measuredPitchGain->setText(QString::number(tuneState->beta[1], 'f', 2));
    measuredYawGain->setText(QString::number(tuneState->beta[2], 'f', 2));

    measuredRollBias->setText(QString::number(tuneState->bias[0], 'f', 3));
    measuredPitchBias->setText(QString::number(tuneState->bias[1], 'f', 3));
    measuredYawBias->setText(QString::number(tuneState->bias[2], 'f', 3));

    rollTau->setText(QString::number(tuneState->tau[0], 'f', 4));
    pitchTau->setText(QString::number(tuneState->tau[1], 'f', 4));
    yawTau->setText(QString::number(tuneState->tau[2], 'f', 4));

    measuredRollNoise->setText(QString::number(tuneState->noise[0], 'f', 2));
    measuredPitchNoise->setText(QString::number(tuneState->noise[1], 'f', 2));
    measuredYawNoise->setText(QString::number(tuneState->noise[2], 'f', 2));

    rollChartView->setRenderHint(QPainter::Antialiasing);
    rollChartView->setChart(makeChart(0));

    pitchChartView->setRenderHint(QPainter::Antialiasing);
    pitchChartView->setChart(makeChart(1));

    yawChartView->setRenderHint(QPainter::Antialiasing);
    yawChartView->setChart(makeChart(2));
}

AutotuneSlidersPage::AutotuneSlidersPage(QWidget *parent, AutotunedValues *autoValues)
    : QWizardPage(parent)
{
    setupUi(this);

    tuneState = autoValues;

    // connect sliders to computation
    connect(rateDamp, &QAbstractSlider::valueChanged, this, &AutotuneSlidersPage::compute);
    connect(rateNoise, &QAbstractSlider::valueChanged, this, &AutotuneSlidersPage::compute);
    connect(cbUseYaw, &QAbstractButton::toggled, this, &AutotuneSlidersPage::compute);
    connect(cbUseOuterKi, &QAbstractButton::toggled, this, &AutotuneSlidersPage::compute);
    connect(cbTuneAlt, &QAbstractButton::toggled, this, &AutotuneSlidersPage::computeThrust);

    connect(btnResetSliders, &QAbstractButton::pressed, this, &AutotuneSlidersPage::resetSliders);
}


void AutotuneSlidersPage::initializePage()
{
    resetSliders();
}

void AutotuneSlidersPage::resetSliders()
{
    rateDamp->setValue(105);
    rateNoise->setValue(10);

    compute();
    computeThrust();
}

bool AutotuneSlidersPage::isComplete() const
{
    return tuneState->converged;
}

void AutotuneSlidersPage::setText(QLabel *lbl, double value, int precision)
{
    if (value < 0) {
        lbl->setText("–");
    } else {
        lbl->setText(QString::number(value, 'f', precision));
    }
}

void AutotuneSlidersPage::computeThrust()
{
    const double baseKp = 0.5;
    const double tauDerate = 0.025;
    const double kiAdjust = 1.5;
    const double outerSlowdown = 0.125;
    const double okpCeiling = 0.9 * GRAVITY / 4.0;

    double hoverThrust = 0;
    double thrustTau = (tuneState->tau[0] + tuneState->tau[1]) / 2 +
        tauDerate;
    /*
     * (Thrust tau is not guaranteed to be axis tau, but it's a pretty
     * good approximation.  We derate it, mostly because we don't trust
     * cfvert to be fast.)
     */
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();

    UAVObjectManager *objMngr = nullptr;
    if (pm) {
        objMngr = pm->getObject<UAVObjectManager>();
    }

    SystemIdent *systemIdent = nullptr;

    if (objMngr) {
        systemIdent = SystemIdent::GetInstance(objMngr);
    }

    if (systemIdent) {
        hoverThrust = systemIdent->getHoverThrottle();
    }

    if ((hoverThrust < 0.05) ||
            (hoverThrust > 0.6) ||
            (thrustTau < 0.005) ||
            (thrustTau > 0.200)) {
        cbTuneAlt->setChecked(false);
        cbTuneAlt->setEnabled(false);
    }

    if (cbTuneAlt->isChecked()) {
        tuneState->vertSpeedKp = (baseKp * hoverThrust / GRAVITY) /
                                    thrustTau;
        tuneState->vertSpeedKi = tuneState->vertSpeedKp /
                                    (kiAdjust * 2 * M_PI * thrustTau);
        tuneState->vertPosKp = outerSlowdown / thrustTau;

        if (tuneState->vertPosKp > okpCeiling) {
            tuneState->vertPosKp = okpCeiling;
        }
    } else {
        tuneState->vertSpeedKp = -1;
        tuneState->vertSpeedKi = -1;
        tuneState->vertPosKp = -1;
    }

    setText(lblAltVelKp, tuneState->vertSpeedKp, 3);
    setText(lblAltVelKi, tuneState->vertSpeedKi, 3);
    setText(lblAltPosKp, tuneState->vertSpeedKi, 3);
}

void AutotuneSlidersPage::compute()
{
    // These three parameters define the desired response properties
    // - rate scale in the fraction of the natural speed of the system
    //   to strive for.
    // - damp is the amount of damping in the system. higher values
    //   make oscillations less likely
    // - ghf is the amount of high frequency gain and limits the influence
    //   of noise

    const double ghf = rateNoise->value() / 1000.0;
    const double damp = rateDamp->value() / 100.0;

    tuneState->damping = damp;
    tuneState->noiseSens = ghf;

    /* Average roll and pitch tau for now. */
    double tau = (tuneState->tau[0] + tuneState->tau[1]) / 2.0;
    double beta_roll = tuneState->beta[0];
    double beta_pitch = tuneState->beta[1];
    double beta_yaw = tuneState->beta[2];

    // First clear out warnings..
    lblWarnings->setText("");

    if (beta_yaw < 6.8) {
        lblWarnings->setText(tr("Unable to auto-calculate yaw gains for this craft."));
        cbUseYaw->setChecked(false);
        cbUseYaw->setEnabled(false);
    }

    bool doYaw = cbUseYaw->isChecked();
    bool doOuterKi = cbUseOuterKi->isChecked();

    double wn = 1 / tau, wn_last = 1 / tau + 10;
    double tau_d = 0, tau_d_last = 1000;

    const int iteration_limit = 100, stability_limit = 5;
    bool converged = false;
    int iterations = 0;
    int stable_iterations = 0;

    while (!converged && (++iterations <= iteration_limit)) {
        double tau_d_roll =
            (2 * damp * tau * wn - 1) / (4 * tau * damp * damp * wn * wn - 2 * damp * wn
                                         - tau * wn * wn + exp(beta_roll) * ghf);
        double tau_d_pitch =
            (2 * damp * tau * wn - 1) / (4 * tau * damp * damp * wn * wn - 2 * damp * wn
                                         - tau * wn * wn + exp(beta_pitch) * ghf);

        // Select the slowest filter property
        tau_d = (tau_d_roll > tau_d_pitch) ? tau_d_roll : tau_d_pitch;
        wn = (tau + tau_d) / (tau * tau_d) / (2 * damp + 2);

        // check for convergence
        if (fabs(tau_d - tau_d_last) <= 0.00001 && fabs(wn - wn_last) <= 0.00001) {
            if (++stable_iterations >= stability_limit)
                converged = true;
        } else {
            stable_iterations = 0;
        }
        tau_d_last = tau_d;
        wn_last = wn;
    }

    tuneState->iterations = iterations;
    tuneState->converged = converged;

    tuneState->derivativeCutoff = 1 / (2 * M_PI * tau_d);
    tuneState->naturalFreq = wn / 2 / M_PI;

    // Set the real pole position. The first pole is quite slow, which
    // prevents the integral being too snappy and driving too much
    // overshoot.
    const double a = ((tau + tau_d) / tau / tau_d - 2 * damp * wn) / 25.0;
    const double b = ((tau + tau_d) / tau / tau_d - 2 * damp * wn - a);

    CONF_ATUNE_QXTLOG_DEBUG("ghf: ", ghf);
    CONF_ATUNE_QXTLOG_DEBUG("wn: ", wn, "tau_d: ", tau_d);
    CONF_ATUNE_QXTLOG_DEBUG("a: ", a, " b: ", b);

    // Calculate the gain for the outer loop by approximating the
    // inner loop as a single order lpf. Set the outer loop to be
    // critically damped;
    const double zeta_o = 1.3;
    tuneState->outerKp = 1 / 4.0 / (zeta_o * zeta_o) / (1 / wn);

    // Except, if this is very high, we may be slew rate limited and pick
    // up oscillation that way.  Fix it with very soft clamping.
    //
    // When we come up with outer KP's of less than 10, things seem safe
    // no matter what.  So beyond 7, start to fade our response.
    //
    // Chosen to have derivative of 1 at 7, and .5 at 10, and never to have
    // derivative change sign.
    if (tuneState->outerKp > 7.0) {
        tuneState->outerKp = 3 * log(tuneState->outerKp - 4) +
            7.0 - 3 * log(3);
    }

    if (doOuterKi) {
        tuneState->outerKp *= 0.95f; // Pick up some margin.
        // Add a zero at 1/15th the innermost bandwidth.
        tuneState->outerKi = 0.75 * tuneState->outerKp / (2 * M_PI * tau * 15.0);
    } else {
        tuneState->outerKi = 0;
    }

    for (int i = 0; i < 3; i++) {
        double beta = exp(tuneState->beta[i]);

        double ki;
        double kp;
        double kd;

        ki = a * b * wn * wn * tau * tau_d / beta;
        kp = tau * tau_d * ((a + b) * wn * wn + 2 * a * b * damp * wn) / beta - ki * tau_d;
        kd = (tau * tau_d * (a * b + wn * wn + (a + b) * 2 * damp * wn) - 1) / beta - kp * tau_d;

        tuneState->kp[i] = kp;
        tuneState->ki[i] = ki;
        tuneState->kd[i] = kd;
    }

    if (!doYaw) {
        tuneState->kp[2] = -1;
        tuneState->ki[2] = -1;
        tuneState->kd[2] = -1;
    }

    // handle non-convergence case.  Takes precedence over all else.
    if (!converged) {
        lblWarnings->setText(tr("<span style=\"color: red\">Error:</span> Tune didn't converge!  "
                                "Check noise and damping sliders."));
    }

    emit completeChanged();

    setText(rollRateKp, tuneState->kp[0], 5);
    setText(rollRateKi, tuneState->ki[0], 5);
    setText(rollRateKd, tuneState->kd[0], 6);

    setText(pitchRateKp, tuneState->kp[1], 5);
    setText(pitchRateKi, tuneState->ki[1], 5);
    setText(pitchRateKd, tuneState->kd[1], 6);

    setText(yawRateKp, tuneState->kp[2], 5);
    setText(yawRateKi, tuneState->ki[2], 5);
    setText(yawRateKd, tuneState->kd[2], 6);

    setText(lblOuterKp, tuneState->outerKp, 2);
    setText(lblOuterKi, tuneState->outerKi, 2);
    setText(derivativeCutoff, tuneState->derivativeCutoff, 1);
    setText(this->wn, tuneState->naturalFreq, 1);
    setText(lblDamp, damp, 2);
    lblNoise->setText(QString::number(ghf * 100, 'f', 1) + " %");
}

AutotuneFinalPage::AutotuneFinalPage(QWidget *parent)
    : QWizardPage(parent)
{
    setupUi(this);

#ifdef Q_OS_MAC
    lblCongrats->setText(lblCongrats->text().replace(tr("\"Finish\""), tr("\"Done\"")));
#endif
}

AutotuneBeginningPage::AutotuneBeginningPage(QWidget *parent,
        bool autoOpened, AutotunedValues *autoValues)
    : QWizardPage(parent)
{
    tuneState = autoValues;
    this->autoOpened = autoOpened;
    dataValid = false;
    setupUi(this);
}

QString AutotuneBeginningPage::tuneValid(bool *okToContinue) const
{
    if ((!tuneState->valid) || (tuneState->tau[0] == 0)) {
        // Invalid / no tune.

        *okToContinue = false;
        return tr("<span style=\"color: red\">It doesn't appear an autotune was successfully "
                  "completed and saved; we are unable to continue.</span>");
    }

    QString retVal;

    *okToContinue = true;

    if (tuneState->tau[0] < 0.005) {
        // Too low of tau to be plausible. (5ms)
        retVal.append(tr("Error: Autotune did not measure valid values for this craft (low tau).  "
                         "Consider slightly lowering the starting roll/pitch rate P values or "
                         "slightly decreasing Motor Input/Output Curve Fit on the output pane."));
        retVal.append("<br/>");
        *okToContinue = false;
    } else if (tuneState->tau[0] < 0.0074) {
        // Probably too low to be real-- 7.4ms-- warn!
        retVal.append(tr("Warning: The tau value measured for this craft is very low."));
        retVal.append("<br/>");
    } else if (tuneState->tau[0] > .240) {
        // Too high of a tau to be plausible / accurate (240ms)-- warn!
        retVal.append(tr("Warning: The tau value measured for this craft is very high."));
        retVal.append("<br/>");
    }

    // Lowest valid gains seen have been 7.9, with most values in the range 9..11
    if (tuneState->beta[0] < 7.25) {
        retVal.append(
            tr("Error: Autotune did not measure valid values for this craft (low roll gain)."));
        retVal.append("<br/>");
        *okToContinue = false;
    }

    if (tuneState->beta[0] < 7.25) {
        retVal.append(
            tr("Error: Autotune did not measure valid values for this craft (low pitch gain)."));
        retVal.append("<br/>");
        *okToContinue = false;
    }

    retVal.replace(QRegExp("(\\w+:)"), "<span style=\"color: red\"><b>\\1</b></span>");

    if (*okToContinue) {
        if (retVal.isEmpty()) {
            retVal.append(tr("Everything checks out, and we're ready to proceed!"));
        } else {
            retVal.append(
                tr("<br/>These warnings may result in an invalid tune.  Proceed with caution."));
        }
    } else {
        retVal.append(
            tr("<br/>Unable to complete the autotune process because of the above error(s)."));
    }

    return retVal;
}

void AutotuneBeginningPage::doDownloadAndProcess()
{
    if (!tuneState->valid) {
        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();

        TelemetryManager *telMngr = pm->getObject<TelemetryManager>();

        if (tuneState->data.isEmpty()) {
            QByteArray *data = telMngr->downloadFile(4, 32768, [&](quint32 progress) {
                        int fraction = (100 * progress) / 32768.0;

                        if (fraction > 80) {
                            fraction = 80;
                        }

                        progressBar->setValue(fraction);
                    });

            if (data) {
                tuneState->data = *data;

                delete data;
            }
        }
    }

    progressBar->setValue(90);

    processAutotuneData();

    progressBar->setValue(100);

    QString initialWarnings = tuneValid(&dataValid);

    status->setText(initialWarnings);

    emit completeChanged();
}

void AutotuneBeginningPage::initializePage()
{
    setTitle(tr("Preparing autotune..."));

    if (autoOpened) {
        subtitle->setText(tr("It looks like you have run a new autotune since you last connected to the flight "
               "controller.  This wizard will assist you in applying a set of autotune "
               "measurements to your aircraft."));
    } else {
        subtitle->setText(tr("This wizard will assist you in applying a set of autotune "
                                  "measurements to your aircraft."));
    }

    progressBar->setRange(0, 100);
    progressBar->setValue(0);

    QMetaObject::invokeMethod(this, "doDownloadAndProcess", Qt::QueuedConnection);
}

bool AutotuneBeginningPage::isComplete() const
{
    return tuneState->valid && dataValid;
}

/* Run a Butterworth biquad filter on a circular buffer.  First go around once
 * to "prime" the filter, then actually filter in place.  This is derived from
 * @glowtape's excellent flight implementation
 */
void AutotuneBeginningPage::biquadFilter(float cutoff, int pts,
        QVector<float> &data)
{
    float f = 1.0f / tan(M_PI * cutoff);
    float q = 1.4142f;

    float y2 = 0, y1 = 0, x2 = 0, x1 = 0;

    float b0 = 1.0f / (1.0f + q * f + f * f);
    float a1 = 2.0f * (f * f - 1.0f) * b0;
    float a2 = -(1.0f - q * f + f * f) * b0;

    for (int i = 0; i < pts; i++) {
        float y = b0 * (data[i] + 2.0f * x1 + x2) + a1 * y1 + a2 * y2;

        y2 = y1;
        y1 = y;

        x2 = x1;
        x1 = data[i];
    }

    for (int i = 0; i < pts; i++) {
        float y = b0 * (data[i] + 2.0f * x1 + x2) + a1 * y1 + a2 * y2;

        y2 = y1;
        y1 = y;

        x2 = x1;
        x1 = data[i];

        data[i] = y;
    }
}

/* Returns number of samples of delay between series */
float AutotuneBeginningPage::getSampleDelay(int pts,
        const QVector<float> &delayed, const QVector<float> &orig,
        int seriesCutoff)
{
    ffft::FFTReal<float> fft(pts);

    /* Convert to frequency domain */
    QVector<float> delayed_fft(pts);
    fft.do_fft(delayed_fft.data(), delayed.data());

    QVector<float> orig_fft(pts);
    fft.do_fft(orig_fft.data(), orig.data());

    /* Now perform a correlation by multiplying -orig_fft* by delayed_fft.
     * The types are all floats here, so we need to do the heavy lifting
     * ourselves.   gfft = x+yi, dfft = u+vi, dfft* = u-vi,
     * -dfft* = -u + vi
     *
     * -dfft* x gfft = (-ux - vy) + (vx - uy)i
     */

    QVector<float> product(pts);

    int fpts = pts / 2;

    // Memory layout here is annoyin'.  All reals, then all imaginaries
    for (int i = 0; i < fpts; i++) {
        float x = delayed_fft[i];
        float y = delayed_fft[i + fpts];
        float u = orig_fft[i];
        float v = orig_fft[i + fpts];

        product[i] = -(u * x) - (v * y);
        product[i + fpts] = (v * x) - (u * y);
    }

    /* Inverse FFT converts this to the time domain */
    QVector<float> prod_time(pts);

    fft.do_ifft(product.data(), prod_time.data());

    /* And we take magnitudes to find tau. */
    QVector<float> mags(fpts);

    int max_idx = 0;
    float max_val = 0;

    for (int i = 0; i < fpts / seriesCutoff; i++) {
        float real = prod_time[i];
        float imag = prod_time[i + fpts];
        mags[i] = sqrt(real * real + imag * imag);

        if (mags[i] > max_val) {
            max_val = mags[i];
            max_idx = i;
        }

        // cout << i << "\t" << mags[i] << "\n";
    }

    // cout << "Maximum mags[" << max_idx << "] = " << max_val << "\n";

    // TODO / optional: interpolate/find a better peak around max_idx.

    return max_idx;
}

bool AutotuneBeginningPage::processAutotuneData()
{
    QByteArray &loadedFile = tuneState->data;

    const at_flash *flash_data = reinterpret_cast<const at_flash *>((const void *)loadedFile);

    unsigned int size = loadedFile.size();

    /* Determine whether we have a sane amount of data, etc. */
    if ((size < sizeof(at_flash)) || (flash_data->hdr.magic != ATFLASH_MAGIC)) {
        return false;
    }

    unsigned int size_expected = sizeof(at_flash)
        + sizeof(at_measurement) * flash_data->hdr.wiggle_points + flash_data->hdr.aux_data_len;

    if (size < size_expected) {
        return false;
    }

    float duration = (float)flash_data->hdr.wiggle_points / flash_data->hdr.sample_rate;

    if ((duration < 0.25f) || (duration > 5.0f)) {
        return false;
    }

    int pts = flash_data->hdr.wiggle_points;

    for (int axis = 0; axis < 3; axis++) {
        QVector<float> gyro_deriv(pts);
        QVector<float> actu_desired(pts);

        for (int i = 0; i < pts; i++) {
            actu_desired[i] = flash_data->data[i].u[axis];
        }

        // Differentiate the gyro data
        for (int i = 1; i < pts; i++) {
            gyro_deriv[i] = flash_data->data[i].y[axis] - flash_data->data[i - 1].y[axis];
        }

        gyro_deriv[0] = flash_data->data[0].y[axis] - flash_data->data[pts - 1].y[axis];

        float sample_tau = getSampleDelay(pts, gyro_deriv, actu_desired,
                (axis == 2) ? 8 : 4);

        float tau = sample_tau / flash_data->hdr.sample_rate;

        biquadFilter(1 / (sample_tau * M_PI * 1.414), pts, actu_desired);

        QVector<float> gyro_sorted = gyro_deriv;
        QVector<float> actu_sorted = actu_desired;

        std::sort(gyro_sorted.begin(), gyro_sorted.end());
        std::sort(actu_sorted.begin(), actu_sorted.end());

        int low_idx = pts * 0.05 + 0.5;
        int high_idx = pts - 1 - low_idx;

        float gyro_span = gyro_sorted[high_idx] - gyro_sorted[low_idx];
        float actu_span = actu_sorted[high_idx] - actu_sorted[low_idx];

        float gain = gyro_span / actu_span * flash_data->hdr.sample_rate;

        float avg = std::accumulate(gyro_deriv.begin(), gyro_deriv.end(), 0.0f) / pts;

        for (int i = 0; i < pts; i++) {
            gyro_deriv[i] = gyro_deriv[i] - avg;
        }

        float avg_act = std::accumulate(actu_desired.begin(), actu_desired.end(), 0.0f) / pts;

        for (int i = 0; i < pts; i++) {
            actu_desired[i] = (actu_desired[i] - avg_act) * (gain / flash_data->hdr.sample_rate);
        }

        tuneState->model[axis] = new QLineSeries(this);
        tuneState->actual[axis] = new QLineSeries(this);

        for (int i = 0; i < pts; i++) {
            int tm = (i * 1000) / flash_data->hdr.sample_rate;

            tuneState->model[axis]->append(tm, actu_desired[i]);
            tuneState->actual[axis]->append(tm, gyro_deriv[i]);
        }

        float bias = avg - avg_act * (gain / flash_data->hdr.sample_rate);

        double noise = 0;

        for (int i = 0; i < pts; i++) {
            noise += (actu_desired[i] - gyro_deriv[i]) * (actu_desired[i] - gyro_deriv[i]);
        }

        noise = sqrt(noise / pts);

        qDebug() << "Series " << axis << ": tau=" << tau << "; gain=" << gain << " (" << log(gain)
                 << "); bias=" << bias << " noise=" << noise << "";

        tuneState->tau[axis] = tau;
        tuneState->beta[axis] = log(gain);
        tuneState->bias[axis] = bias;
        tuneState->noise[axis] = noise;
    }

    tuneState->valid = true;

    return true;
}
