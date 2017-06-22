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

#include <QtAlgorithms>
#include <QCryptographicHash>
#include <QDebug>
#include <QFileDialog>
#include <QStringList>
#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QList>
#include <QMessageBox>
#include <QClipboard>
#include <QVector>
#include <QWizard>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

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
                                                   AutotunedValues *av)
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
    roll_ident["gain"] = av->beta[0];
    roll_ident["bias"] = av->bias[0];
    roll_ident["noise"] = av->noise[0];
    roll_ident["tau"] = av->tau[0];
    identification["roll"] = roll_ident;

    QJsonObject pitch_ident;
    pitch_ident["gain"] = av->beta[1];
    pitch_ident["bias"] = av->bias[1];
    pitch_ident["noise"] = av->noise[1];
    pitch_ident["tau"] = av->tau[1];
    identification["pitch"] = pitch_ident;

    QJsonObject yaw_ident;
    yaw_ident["gain"] = av->beta[2];
    yaw_ident["bias"] = av->bias[2];
    yaw_ident["noise"] = av->noise[2];
    yaw_ident["tau"] = av->tau[2];
    identification["yaw"] = yaw_ident;

    identification["tau"] = av->tau[0];
    json["identification"] = identification;

    QJsonObject tuning, parameters, computed, misc;
    parameters["damping"] = av->damping;
    parameters["noiseSensitivity"] = av->noiseSens;

    tuning["parameters"] = parameters;
    computed["naturalFrequency"] = av->naturalFreq;
    computed["derivativeCutoff"] = av->derivativeCutoff;
    computed["converged"] = av->converged;
    computed["iterations"] = av->iterations;

    QJsonObject gains;
    QJsonObject roll_gain, pitch_gain, yaw_gain, outer_gain;
    roll_gain["kp"] = av->kp[0];
    roll_gain["ki"] = av->ki[0];
    roll_gain["kd"] = av->kd[0];
    gains["roll"] = roll_gain;
    pitch_gain["kp"] = av->kp[1];
    pitch_gain["ki"] = av->ki[1];
    pitch_gain["kd"] = av->kd[1];
    gains["pitch"] = pitch_gain;
    yaw_gain["kp"] = av->kp[2];
    yaw_gain["ki"] = av->ki[2];
    yaw_gain["kd"] = av->kd[2];
    gains["yaw"] = yaw_gain;
    outer_gain["kp"] = av->outerKp;
    outer_gain["ki"] = av->outerKi;
    gains["outer"] = outer_gain;
    computed["gains"] = gains;
    tuning["computed"] = computed;
    json["tuning"] = tuning;

    json["rawSettings"] = rawSettings;

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

    AutotunedValues vals;
    memset(&vals, 0, sizeof(vals));
    vals.data = new QByteArray(file.readAll());

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
    AutotunedValues av;

    if (precalc_vals) {
        av = *precalc_vals;
    } else {
        memset(&av, 0, sizeof(av));
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
        if (!stabilizationSettings)
            return;

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

    this->av = autoValues;
}

void AutotuneMeasuredPropertiesPage::initializePage()
{
    measuredRollGain->setText(QString::number(av->beta[0], 'f', 2));
    measuredPitchGain->setText(QString::number(av->beta[1], 'f', 2));
    measuredYawGain->setText(QString::number(av->beta[2], 'f', 2));

    measuredRollBias->setText(QString::number(av->bias[0], 'f', 3));
    measuredPitchBias->setText(QString::number(av->bias[1], 'f', 3));
    measuredYawBias->setText(QString::number(av->bias[2], 'f', 3));

    double tau = av->tau[0];

    rollTau->setText(QString::number(tau, 'f', 4));
    pitchTau->setText(QString::number(tau, 'f', 4));

    measuredRollNoise->setText(QString::number(av->noise[0], 'f', 2));
    measuredPitchNoise->setText(QString::number(av->noise[1], 'f', 2));
    measuredYawNoise->setText(QString::number(av->noise[2], 'f', 2));
}

AutotuneSlidersPage::AutotuneSlidersPage(QWidget *parent, AutotunedValues *autoValues)
    : QWizardPage(parent)
{
    setupUi(this);

    av = autoValues;

    // connect sliders to computation
    connect(rateDamp, &QAbstractSlider::valueChanged, this, &AutotuneSlidersPage::compute);
    connect(rateNoise, &QAbstractSlider::valueChanged, this, &AutotuneSlidersPage::compute);
    connect(cbUseYaw, &QAbstractButton::toggled, this, &AutotuneSlidersPage::compute);
    connect(cbUseOuterKi, &QAbstractButton::toggled, this, &AutotuneSlidersPage::compute);

    connect(btnResetSliders, &QAbstractButton::pressed, this, &AutotuneSlidersPage::resetSliders);

    compute();
}

void AutotuneSlidersPage::resetSliders()
{
    rateDamp->setValue(105);
    rateNoise->setValue(10);

    compute();
}

bool AutotuneSlidersPage::isComplete() const
{
    return av->converged;
}

void AutotuneSlidersPage::setText(QLabel *lbl, double value, int precision)
{
    if (value < 0) {
        lbl->setText("–");
    } else {
        lbl->setText(QString::number(value, 'f', precision));
    }
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

    av->damping = damp;
    av->noiseSens = ghf;

    double tau = av->tau[0];
    double beta_roll = av->beta[0];
    double beta_pitch = av->beta[1];
    double beta_yaw = av->beta[2];

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

    av->iterations = iterations;
    av->converged = converged;

    av->derivativeCutoff = 1 / (2 * M_PI * tau_d);
    av->naturalFreq = wn / 2 / M_PI;

    // Set the real pole position. The first pole is quite slow, which
    // prevents the integral being too snappy and driving too much
    // overshoot.
    const double a = ((tau + tau_d) / tau / tau_d - 2 * damp * wn) / 20.0;
    const double b = ((tau + tau_d) / tau / tau_d - 2 * damp * wn - a);

    CONF_ATUNE_QXTLOG_DEBUG("ghf: ", ghf);
    CONF_ATUNE_QXTLOG_DEBUG("wn: ", wn, "tau_d: ", tau_d);
    CONF_ATUNE_QXTLOG_DEBUG("a: ", a, " b: ", b);

    // Calculate the gain for the outer loop by approximating the
    // inner loop as a single order lpf. Set the outer loop to be
    // critically damped;
    const double zeta_o = 1.3;
    av->outerKp = 1 / 4.0 / (zeta_o * zeta_o) / (1 / wn);

    // Except, if this is very high, we may be slew rate limited and pick
    // up oscillation that way.  Fix it with very soft clamping.
    // MaximumRate defaults to 350, 6.5 corresponds to where we begin
    // clamping rate ourselves.  ESCs, etc, it depends upon gains
    // and any pre-emphasis they do.   Still give ourselves partial credit
    // for inner loop bandwidth.
    if (av->outerKp > 6.5) {
        av->outerKp = 6.5 - sqrt(6.5) + sqrt(av->outerKp);
    }

    if (doOuterKi) {
        av->outerKp *= 0.95f; // Pick up some margin.
        // Add a zero at 1/15th the innermost bandwidth.
        av->outerKi = 0.75 * av->outerKp / (2 * M_PI * tau * 15.0);
    } else {
        av->outerKi = 0;
    }

    for (int i = 0; i < 2; i++) {
        double beta = exp(av->beta[i]);

        double ki;
        double kp;
        double kd;

        ki = a * b * wn * wn * tau * tau_d / beta;
        kp = tau * tau_d * ((a + b) * wn * wn + 2 * a * b * damp * wn) / beta - ki * tau_d;
        kd = (tau * tau_d * (a * b + wn * wn + (a + b) * 2 * damp * wn) - 1) / beta - kp * tau_d;

        av->kp[i] = kp;
        av->ki[i] = ki;
        av->kd[i] = kd;
    }

    if (doYaw) {
        // Don't take yaw beta completely seriously.  Why?
        // 1) It's got two different time constants and magnitudes of
        // effect (reaction wheel vs. drag).  Don't want to overcontrol.
        // 2) Far better to be undertuned on yaw than to get into weird
        // scenarios from coupling between axes.  If yaw is far less
        // powerful than other axes, even a small amount of nonlinearity
        // or cross-axis coupling will excite pitch and roll.
        double scale = exp(0.6 * (av->beta[0] - av->beta[2]));
        av->kp[2] = av->kp[0] * scale;
        av->ki[2] = 0.8 * av->ki[0] * scale;
        av->kd[2] = 0.8 * av->kd[0] * scale;
    } else {
        av->kp[2] = -1;
        av->ki[2] = -1;
        av->kd[2] = -1;
    }

    // handle non-convergence case.  Takes precedence over all else.
    if (!converged) {
        lblWarnings->setText(tr("<span style=\"color: red\">Error:</span> Tune didn't converge!  "
                                "Check noise and damping sliders."));
    }

    emit completeChanged();

    setText(rollRateKp, av->kp[0], 5);
    setText(rollRateKi, av->ki[0], 5);
    setText(rollRateKd, av->kd[0], 6);

    setText(pitchRateKp, av->kp[1], 5);
    setText(pitchRateKi, av->ki[1], 5);
    setText(pitchRateKd, av->kd[1], 6);

    setText(yawRateKp, av->kp[2], 5);
    setText(yawRateKi, av->ki[2], 5);
    setText(yawRateKd, av->kd[2], 6);

    setText(lblOuterKp, av->outerKp, 2);
    setText(lblOuterKi, av->outerKi, 2);
    setText(derivativeCutoff, av->derivativeCutoff, 1);
    setText(this->wn, av->naturalFreq, 1);
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
    av = autoValues;
    this->autoOpened = autoOpened;
    dataValid = false;
    setupUi(this);
}

QString AutotuneBeginningPage::tuneValid(bool *okToContinue) const
{
    if ((!av->valid) || (av->tau[0] == 0)) {
        // Invalid / no tune.

        *okToContinue = false;
        return tr("<span style=\"color: red\">It doesn't appear an autotune was successfully "
                  "completed and saved; we are unable to continue.</span>");
    }

    QString retVal;

    *okToContinue = true;

    if (av->tau[0] < 0.005) {
        // Too low of tau to be plausible. (5ms)
        retVal.append(tr("Error: Autotune did not measure valid values for this craft (low tau).  "
                         "Consider slightly lowering the starting roll/pitch rate P values or "
                         "slightly decreasing Motor Input/Output Curve Fit on the output pane."));
        retVal.append("<br/>");
        *okToContinue = false;
    } else if (av->tau[0] < 0.0074) {
        // Probably too low to be real-- 7.4ms-- warn!
        retVal.append(tr("Warning: The tau value measured for this craft is very low."));
        retVal.append("<br/>");
    } else if (av->tau[0] > .240) {
        // Too high of a tau to be plausible / accurate (240ms)-- warn!
        retVal.append(tr("Warning: The tau value measured for this craft is very high."));
        retVal.append("<br/>");
    }

    // Lowest valid gains seen have been 7.9, with most values in the range 9..11
    if (av->beta[0] < 7.25) {
        retVal.append(
            tr("Error: Autotune did not measure valid values for this craft (low roll gain)."));
        retVal.append("<br/>");
        *okToContinue = false;
    }

    if (av->beta[0] < 7.25) {
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
    if (!av->valid) {
        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();

        TelemetryManager *telMngr = pm->getObject<TelemetryManager>();

        if (!av->data) {
            av->data = telMngr->downloadFile(4, 32768, [&](quint32 progress) {
                        int fraction = (100 * progress) / 32768.0;

                        if (fraction > 80) {
                            fraction = 80;
                        }

                        progressBar->setValue(fraction);
                    });
        }
    }

    progressBar->setValue(90);

    if (av->data) {
        processAutotuneData();
    }

    progressBar->setValue(100);

    QString initialWarnings = tuneValid(&dataValid);

    status->setText(initialWarnings);
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
    return av->valid && dataValid;
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
        const QVector<float> &delayed, const QVector<float> &orig)
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

    for (int i = 0; i < fpts; i++) {
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
    QByteArray *loadedFile = av->data;

    const at_flash *flash_data = reinterpret_cast<const at_flash *>((const void *)*loadedFile);

    unsigned int size = loadedFile->size();

    AutotunedValues vals = {};

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

        float sample_tau = getSampleDelay(pts, gyro_deriv, actu_desired);

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

        float bias = avg - avg_act * (gain / flash_data->hdr.sample_rate);

        double noise = 0;

        for (int i = 0; i < pts; i++) {
            noise += (actu_desired[i] - gyro_deriv[i]) * (actu_desired[i] - gyro_deriv[i]);
        }

        noise = sqrt(noise / pts);

        qDebug() << "Series " << axis << ": tau=" << tau << "; gain=" << gain << " (" << log(gain)
                 << "); bias=" << bias << " noise=" << noise << "";

        vals.tau[axis] = tau;
        vals.beta[axis] = log(gain);
        vals.bias[axis] = bias;
        vals.noise[axis] = noise;
    }

    vals.valid = true;

    *av = vals;

    return true;
}
