/**
 ******************************************************************************
 *
 * @file       configautotunewidget.cpp
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

#include "configautotunewidget.h"

#include <QCryptographicHash>
#include <QDebug>
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
#define CONF_ATUNE_QXTLOG_DEBUG(...) qDebug()<<__VA_ARGS__
#else  // CONF_ATUNE_DEBUG
#define CONF_ATUNE_QXTLOG_DEBUG(...)
#endif	// CONF_ATUNE_DEBUG


const QString ConfigAutotuneWidget::databaseUrl = QString("http://dronin-autotown.appspot.com/storeTune");

ConfigAutotuneWidget::ConfigAutotuneWidget(ConfigGadgetWidget *parent) :
    ConfigTaskWidget(parent)
{
    parentConfigWidget = parent;
    m_autotune = new Ui_AutotuneWidget();
    m_autotune->setupUi(this);

    // Connect automatic signals
    autoLoadWidgets();
    disableMouseWheelEvents();

    // Whenever any value changes compute new potential stabilization settings
    connect(m_autotune->rateDamp, SIGNAL(valueChanged(int)), this, SLOT(recomputeStabilization()));
    connect(m_autotune->rateNoise, SIGNAL(valueChanged(int)), this, SLOT(recomputeStabilization()));

    connect(m_autotune->cbUseYaw, SIGNAL(toggled(bool)), this, SLOT(onYawTuneToggled(bool)));
    connect(m_autotune->cbUseOuterKi, SIGNAL(toggled(bool)), this, SLOT(recomputeStabilization()));

    addUAVObject(ModuleSettings::NAME);

    SystemIdent *systemIdent = SystemIdent::GetInstance(getObjectManager());

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    utilMngr = pm->getObject<UAVObjectUtilManager>();

    addWidget(m_autotune->enableAutoTune);

    connect(systemIdent, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(recomputeStabilization()));

    // Connect the apply button for the stabilization settings
    connect(m_autotune->useComputedValues, SIGNAL(pressed()), this, SLOT(saveStabilization()));

    connect(m_autotune->shareDataPB, SIGNAL(pressed()),this, SLOT(onShareData()));
    connect(m_autotune->btnResetSliders, SIGNAL(pressed()), this, SLOT(resetSliders()));

    setNotMandatory(systemIdent->getName());

    // force defaults in-case somebody tries to change them in UI and forgets to update this func
    resetSliders();
    setApplyEnabled(false);
}

/**
  * Apply the stabilization settings computed
  */
void ConfigAutotuneWidget::saveStabilization()
{
    StabilizationSettings *stabilizationSettings = StabilizationSettings::GetInstance(getObjectManager());
    Q_ASSERT(stabilizationSettings);
    if(!stabilizationSettings)
        return;

    // Check the settings are reasonable, or if not have the
    // user confirm they want to continue.
    SystemIdent *systemIdent = SystemIdent::GetInstance(getObjectManager());
    Q_ASSERT(systemIdent);
    if(!systemIdent)
        return;

    // Make sure to recompute in case the other stab settings changed since
    // the last time
    recomputeStabilization();

    if (approveSettings(systemIdent->getData()) == false)
        return;

    // Apply this data to the board
    stabilizationSettings->setData(stabSettings);
    stabilizationSettings->updated();

    QMessageBox::information(this,tr("Tune values updated"),
            tr("The calculated autotune values have been entered on the "
                "stabilization settings pane and applied to RAM on the "
	        "flight controller.  Please review and then "
                "permanently save them.\n\n"
                "You will now be taken to the stabilization settings pane."));

    parentConfigWidget->changeTab(ConfigGadgetWidget::stabilization);
}

void ConfigAutotuneWidget::onShareData()
{
    autotuneShareForm = new AutotuneShareForm();
    autotuneShareForm->setAttribute(Qt::WA_DeleteOnClose, true);
    connect(autotuneShareForm, SIGNAL(ClipboardRequest()), this, SLOT(onShareToClipboard()));
    connect(autotuneShareForm, SIGNAL(DatabaseRequest()), this, SLOT(onShareToDatabase()));

    QString currentFrameType;
    SystemSettings *sysSettings = SystemSettings::GetInstance(getObjectManager());
    if(sysSettings) {
        UAVObjectField *frameType = sysSettings->getField("AirframeType");
        if(frameType) {
            autotuneShareForm->setVehicleTypeOptions(frameType->getOptions());
            currentFrameType = frameType->getValue().toString();
        }
    }
    // would be nice to set board type dropdown too but too hard to get a list (for now)

    // fetch last shared data
    loadUserData();

    // we will overwrite some things though
    Core::IBoardType *board = utilMngr->getBoardType();
    if(board) {
        autotuneShareForm->setBoardType(board->shortName());
        autotuneShareForm->disableBoardType(true);
    }
    if(!currentFrameType.isNull()) {
        autotuneShareForm->setVehicleType(currentFrameType);
        autotuneShareForm->disableVehicleType(true);
    }

    autotuneShareForm->show();
    autotuneShareForm->raise();
    autotuneShareForm->activateWindow();
}

void ConfigAutotuneWidget::onShareToDatabase()
{
    autotuneShareForm->disableDatabase(true);
    // save data for next time the form is used
    saveUserData();

    autotuneShareForm->hideProgress(false);
    autotuneShareForm->setProgress(0, 0);

    QJsonDocument json = getResultsJson();

    QUrl url(databaseUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    connect(manager, SIGNAL(finished(QNetworkReply*)),
                this, SLOT(onShareToDatabaseComplete(QNetworkReply*)));

    QNetworkReply *reply = manager->post(request, json.toJson());
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), autotuneShareForm, SLOT(setProgress(qint64,qint64)));
}

void ConfigAutotuneWidget::onShareToDatabaseComplete(QNetworkReply *reply)
{
    disconnect(reply, SIGNAL(uploadProgress(qint64,qint64)), autotuneShareForm, SLOT(setProgress(qint64,qint64)));
    if(reply->error() != QNetworkReply::NoError) {
        qWarning() << "[ConfigAutotuneWidget::onShareToDatabaseComplete]HTTP Error: " << reply->errorString();
        autotuneShareForm->hideProgress(true);
        autotuneShareForm->disableDatabase(false);
        QMessageBox msgBox;
        msgBox.setText(tr("An error occured!"));
        msgBox.setInformativeText(tr("Your results could not be shared to the database. Please try again later."));
        msgBox.setDetailedText(QString("URL: %1\nReply: %2\n")
                               .arg(reply->url().toString())
                               .arg(reply->errorString()));
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
    }
    else {
        autotuneShareForm->setProgress(100, 100);
        // database share button remains disabled, no need to send twice
    }
    reply->deleteLater();
}

void ConfigAutotuneWidget::onShareToClipboard()
{
    // save data for next time the form is used
    saveUserData();

    QString message = getResultsPlainText();

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(message);
}

void ConfigAutotuneWidget::loadUserData()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Core::Internal::GeneralSettings *settings = pm->getObject<Core::Internal::GeneralSettings>();

    autotuneShareForm->setObservations(settings->getObservations());
    autotuneShareForm->setVehicleType(settings->getVehicleType());
    autotuneShareForm->setBoardType(settings->getBoardType());
    autotuneShareForm->setWeight(settings->getWeight());
    autotuneShareForm->setVehicleSize(settings->getVehicleSize());
    autotuneShareForm->setBatteryCells(settings->getBatteryCells());
    autotuneShareForm->setMotors(settings->getMotors());
    autotuneShareForm->setESCs(settings->getESCs());
    autotuneShareForm->setProps(settings->getProps());
}

void ConfigAutotuneWidget::saveUserData()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Core::Internal::GeneralSettings *settings = pm->getObject<Core::Internal::GeneralSettings>();
    settings->setObservations(autotuneShareForm->getObservations());
    settings->setVehicleType(autotuneShareForm->getVehicleType());
    settings->setBoardType(autotuneShareForm->getBoardType());
    settings->setWeight(autotuneShareForm->getWeight());
    settings->setVehicleSize(autotuneShareForm->getVehicleSize());
    settings->setBatteryCells(autotuneShareForm->getBatteryCells());
    settings->setMotors(autotuneShareForm->getMotors());
    settings->setESCs(autotuneShareForm->getESCs());
    settings->setProps(autotuneShareForm->getProps());
}

/**
 * @brief ConfigAutotuneWidget::approveSettings
 * @param data The system ident values
 * @return True if these are ok, False if not
 *
 * For values that seem potentially problematic
 * a dialog will explicitly check the user wants
 * to apply them.
 */
bool ConfigAutotuneWidget::approveSettings(
        SystemIdent::DataFields systemIdentData)
{
    // Check the axis gains
    if (systemIdentData.Beta[SystemIdent::BETA_ROLL] < 6 ||
        systemIdentData.Beta[SystemIdent::BETA_PITCH] < 6) {

        int ans = QMessageBox::warning(this,tr("Extreme values"),
                                     tr("Your roll or pitch gain was lower than expected. This will result in large PID values. "
                                                           "Do you still want to proceed?"), QMessageBox::Yes,QMessageBox::No);
        if (ans == QMessageBox::No)
            return false;
    }

    // Check the response speed
    if (exp(systemIdentData.Tau) > 0.1) {

        int ans = QMessageBox::warning(this,tr("Extreme values"),
                                     tr("Your estimated response speed (tau) is slower than normal. This will result in large PID values. "
                                                           "Do you still want to proceed?"), QMessageBox::Yes,QMessageBox::No);
        if (ans == QMessageBox::No)
            return false;
    } else if (exp(systemIdentData.Tau) < 0.008) {

        int ans = QMessageBox::warning(this,tr("Extreme values"),
                                     tr("Your estimated response speed (tau) is faster than normal. This will result in large PID values. "
                                                           "Do you still want to proceed?"), QMessageBox::Yes,QMessageBox::No);
        if (ans == QMessageBox::No)
            return false;
    }

    return true;
}

/**
  * Called whenever the gain ratios or measured values
  * are changed
  */
void ConfigAutotuneWidget::recomputeStabilization()
{
    SystemIdent *systemIdent = SystemIdent::GetInstance(getObjectManager());
    Q_ASSERT(systemIdent);
    if(!systemIdent)
        return;

    StabilizationSettings *stabilizationSettings = StabilizationSettings::GetInstance(getObjectManager());
    Q_ASSERT(stabilizationSettings);
    if(!stabilizationSettings)
        return;

    SystemIdent::DataFields systemIdentData = systemIdent->getData();
    stabSettings = stabilizationSettings->getData();

    // These three parameters define the desired response properties
    // - rate scale in the fraction of the natural speed of the system
    //   to strive for.
    // - damp is the amount of damping in the system. higher values
    //   make oscillations less likely
    // - ghf is the amount of high frequency gain and limits the influence
    //   of noise
    const double ghf = m_autotune->rateNoise->value() / 1000.0;
    const double damp = m_autotune->rateDamp->value() / 100.0;

    double tau = exp(systemIdentData.Tau);
    double beta_roll = systemIdentData.Beta[SystemIdent::BETA_ROLL];
    double beta_pitch = systemIdentData.Beta[SystemIdent::BETA_PITCH];

    double wn = 1/tau, wn_last = 1/tau + 10;
    double tau_d = 0, tau_d_last = 1000;

    const int iteration_limit = 100, stability_limit = 5;
    converged = false;
    iterations = 0;
    int stable_iterations = 0;

    while (++iterations <= iteration_limit && !converged) {
        double tau_d_roll = (2*damp*tau*wn - 1)/(4*tau*damp*damp*wn*wn - 2*damp*wn - tau*wn*wn + exp(beta_roll)*ghf);
        double tau_d_pitch = (2*damp*tau*wn - 1)/(4*tau*damp*damp*wn*wn - 2*damp*wn - tau*wn*wn + exp(beta_pitch)*ghf);

        // Select the slowest filter property
        tau_d = (tau_d_roll > tau_d_pitch) ? tau_d_roll : tau_d_pitch;
        wn = (tau + tau_d) / (tau*tau_d) / (2 * damp + 2);

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
    --iterations; // make the number right for tune share etc.

    setApplyEnabled(converged);

    // Set the real pole position. The first pole is quite slow, which
    // prevents the integral being too snappy and driving too much
    // overshoot.
    const double a = ((tau+tau_d) / tau / tau_d - 2 * damp * wn) / 20.0;
    const double b = ((tau+tau_d) / tau / tau_d - 2 * damp * wn - a);

    CONF_ATUNE_QXTLOG_DEBUG("ghf: ", ghf);
    CONF_ATUNE_QXTLOG_DEBUG("wn: ", wn, "tau_d: ", tau_d);
    CONF_ATUNE_QXTLOG_DEBUG("a: ", a, " b: ", b);

    // Calculate the gain for the outer loop by approximating the
    // inner loop as a single order lpf. Set the outer loop to be
    // critically damped;
    const double zeta_o = 1.3;
    const double kp_o = 1 / 4.0 / (zeta_o * zeta_o) / (1/wn);
    const double ki_o = (m_autotune->cbUseOuterKi->isChecked()) ? (0.75 * kp_o / (2 * M_PI * tau * 10.0)) : 0.0;

    // For now just run over roll and pitch
    for (int i = 0; i < 3; i++) {
        double beta = exp(systemIdentData.Beta[i]);

        double ki;
        double kp;
        double kd;

        switch (i) {
        case 0: // Roll
        case 1: // Pitch
            ki = a * b * wn * wn * tau * tau_d / beta;
            kp = tau * tau_d * ((a+b)*wn*wn + 2*a*b*damp*wn) / beta - ki*tau_d;
            kd = (tau * tau_d * (a*b + wn*wn + (a+b)*2*damp*wn) - 1) / beta - kp * tau_d;
            break;
        case 2: // Yaw
            beta = exp(0.6 * (systemIdentData.Beta[SystemIdent::BETA_PITCH] - systemIdentData.Beta[SystemIdent::BETA_YAW]));
            kp = stabSettings.PitchRatePID[StabilizationSettings::PITCHRATEPID_KP] * beta;
            ki = 0.8 * stabSettings.PitchRatePID[StabilizationSettings::PITCHRATEPID_KI] * beta;
            kd = 0.8 * stabSettings.PitchRatePID[StabilizationSettings::PITCHRATEPID_KD] * beta;
            break;
        }

        switch(i) {
        case 0: // Roll
            stabSettings.RollRatePID[StabilizationSettings::ROLLRATEPID_KP] = kp;
            stabSettings.RollRatePID[StabilizationSettings::ROLLRATEPID_KI] = ki;
            stabSettings.RollRatePID[StabilizationSettings::ROLLRATEPID_KD] = kd;
            stabSettings.RollPI[StabilizationSettings::ROLLPI_KP] = kp_o;
            stabSettings.RollPI[StabilizationSettings::ROLLPI_KI] = ki_o;
            break;
        case 1: // Pitch
            stabSettings.PitchRatePID[StabilizationSettings::PITCHRATEPID_KP] = kp;
            stabSettings.PitchRatePID[StabilizationSettings::PITCHRATEPID_KI] = ki;
            stabSettings.PitchRatePID[StabilizationSettings::PITCHRATEPID_KD] = kd;
            stabSettings.PitchPI[StabilizationSettings::PITCHPI_KP] = kp_o;
            stabSettings.PitchPI[StabilizationSettings::PITCHPI_KI] = ki_o;
            break;
        case 2: // Yaw
            if (m_autotune->cbUseYaw->isChecked() && systemIdentData.Beta[SystemIdent::BETA_YAW] >= 6.3) {
                stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KP] = kp;
                stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KI] = ki;
                stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KD] = kd;
                //stabSettings.YawPI[StabilizationSettings::YAWPI_KP] = kp_o;
                //stabSettings.YawPI[StabilizationSettings::YAWPI_KI] = ki_o;
            }
            break;
        }
    }
    stabSettings.DerivativeCutoff = 1 / (2*M_PI*tau_d);

    // Display these computed settings
    m_autotune->rollRateKp->setText(QString::number(stabSettings.RollRatePID[StabilizationSettings::ROLLRATEPID_KP]));
    m_autotune->rollRateKi->setText(QString::number(stabSettings.RollRatePID[StabilizationSettings::ROLLRATEPID_KI]));
    m_autotune->rollRateKd->setText(QString::number(stabSettings.RollRatePID[StabilizationSettings::ROLLRATEPID_KD]));
    m_autotune->pitchRateKp->setText(QString::number(stabSettings.PitchRatePID[StabilizationSettings::PITCHRATEPID_KP]));
    m_autotune->pitchRateKi->setText(QString::number(stabSettings.PitchRatePID[StabilizationSettings::PITCHRATEPID_KI]));
    m_autotune->pitchRateKd->setText(QString::number(stabSettings.PitchRatePID[StabilizationSettings::PITCHRATEPID_KD]));
    if (m_autotune->cbUseYaw->isChecked() && systemIdentData.Beta[SystemIdent::BETA_YAW] >= 6.3) {
        m_autotune->yawRateKp->setText(QString::number(stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KP]));
        m_autotune->yawRateKi->setText(QString::number(stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KI]));
        m_autotune->yawRateKd->setText(QString::number(stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KD]));
    } else {
        m_autotune->yawRateKp->setText("-");
        m_autotune->yawRateKi->setText("-");
        m_autotune->yawRateKd->setText("-");
    }
    m_autotune->lblOuterKp->setText(QString::number(stabSettings.RollPI[StabilizationSettings::ROLLPI_KP]));
    m_autotune->lblOuterKi->setText(QString::number(stabSettings.RollPI[StabilizationSettings::ROLLPI_KI]));

    m_autotune->derivativeCutoff->setText(QString::number(stabSettings.DerivativeCutoff));
    m_autotune->rollTau->setText(QString::number(tau,'g',3));
    m_autotune->pitchTau->setText(QString::number(tau,'g',3));
    m_autotune->wn->setText(QString::number(wn / 2 / M_PI, 'f', 1));
    m_autotune->lblDamp->setText(QString::number(damp, 'f', 2));
    m_autotune->lblNoise->setText(QString::number(ghf * 100, 'f', 1) + " %");

}

void ConfigAutotuneWidget::refreshWidgetsValues(UAVObject *obj)
{
    ModuleSettings *moduleSettings = ModuleSettings::GetInstance(getObjectManager());
    if(obj==moduleSettings)
    {
        bool dirtyBack=isDirty();
        ModuleSettings::DataFields moduleSettingsData = moduleSettings->getData();
        m_autotune->enableAutoTune->setChecked(
            moduleSettingsData.AdminState[ModuleSettings::ADMINSTATE_AUTOTUNE] == ModuleSettings::ADMINSTATE_ENABLED);
        setDirty(dirtyBack);
    }
    ConfigTaskWidget::refreshWidgetsValues(obj);
}
void ConfigAutotuneWidget::updateObjectsFromWidgets()
{
    ModuleSettings *moduleSettings = ModuleSettings::GetInstance(getObjectManager());
    ModuleSettings::DataFields moduleSettingsData = moduleSettings->getData();
    moduleSettingsData.AdminState[ModuleSettings::ADMINSTATE_AUTOTUNE] =
         m_autotune->enableAutoTune->isChecked() ? ModuleSettings::ADMINSTATE_ENABLED : ModuleSettings::ADMINSTATE_DISABLED;
    moduleSettings->setData(moduleSettingsData);
    ConfigTaskWidget::updateObjectsFromWidgets();
}

/**
 * @brief ConfigAutotuneWidget::generateResultsPlainText
 * @return
 */
QString ConfigAutotuneWidget::getResultsPlainText()
{
    deviceDescriptorStruct firmware;
    utilMngr->getBoardDescriptionStruct(firmware);

    SystemSettings *sysSettings = SystemSettings::GetInstance(getObjectManager());
    UAVObjectField *afTypeField = sysSettings->getField("AirframeType");
    QString afType;
    if(afTypeField) {
        QStringList vehicleTypes = afTypeField->getOptions();
        afType = vehicleTypes[sysSettings->getAirframeType()];
    }

    QString message0 = tr(
                "Flight controller:\t%0\n"
                "Firmware tag:\t\t%1\n"
                "Firmware commit:\t%2\n"
                "Firmware date:\t\t%3\n\n"
                "Vehicle description:\n"
                "Type:\t\t\t%4\n"
                "Weight (AUW):\t%5 g\n"
                "Size:\t\t\t%6\n"
                "Battery:\t\t%7S\n"
                "Motors:\t\t\t%8\n"
                "ESCs:\t\t\t%9\n"
                "Propellers:\t\t%10\n\n"
                "Observations:\n%11\n\n")
            .arg(autotuneShareForm->getBoardType()) // 0
            .arg(firmware.gitTag) // 1
            .arg(firmware.gitHash.left(7)) // 2
            .arg(firmware.gitDate + " UTC") // 3
            .arg(afType) // 4
            .arg(autotuneShareForm->getWeight()) // 5
            .arg(autotuneShareForm->getVehicleSize()) // 6
            .arg(autotuneShareForm->getBatteryCells()) // 7
            .arg(autotuneShareForm->getMotors()) // 8
            .arg(autotuneShareForm->getESCs()) // 9
            .arg(autotuneShareForm->getProps()) // 10
            .arg(autotuneShareForm->getObservations()); // 11
    QString message1 = tr(
                "Measured properties:\n"
                "\t\tGain\t\tBias\t\tNoise\n"
                "Roll:\t%0\t\t%1\t%2\n"
                "Pitch:\t%3\t\t%4\t%5\n"
                "Tau:\t%6 ms\n\n"
                "Tuning aggressiveness:\n"
                "Damping:\t\t\t%7\n"
                "Noise sensitivity:\t%8\n"
                "Natural frequency:\t%9\n\n")
            .arg(m_autotune->measuredRollGain->text()) // 0
            .arg(m_autotune->measuredRollBias->text()) // 1
            .arg(m_autotune->measuredRollNoise->text()) // 2
            .arg(m_autotune->measuredPitchGain->text()) // 3
            .arg(m_autotune->measuredPitchBias->text()) // 4
            .arg(m_autotune->measuredPitchNoise->text()) // 5
            .arg(m_autotune->rollTau->text().toDouble()*1000.0) // 6
            .arg(m_autotune->lblDamp->text()) // 7
            .arg(m_autotune->lblNoise->text()) // 8
            .arg(m_autotune->wn->text()); // 9
    QString message2 = tr(
                "Computed values:\n"
                "Converged:\t%0\n"
                "\t\t\tRateKp\t\tRateKi\t\tRateKd\n"
                "Roll:\t\t%1\t%2\t%3\n"
                "Pitch:\t\t%4\t%5\t%6\n"
                "Yaw:\t\t%7\t%8\t%9\n"
                "Outer:\t\t%10\t\t%11\t\t-\n"
                "Derivative cutoff:\t%12")
            .arg(converged ? "Yes" : "No") // 0
            .arg(m_autotune->rollRateKp->text()) // 1
            .arg(m_autotune->rollRateKi->text()) // 2
            .arg(m_autotune->rollRateKd->text()) // 3
            .arg(m_autotune->pitchRateKp->text()) // 4
            .arg(m_autotune->pitchRateKi->text()) // 5
            .arg(m_autotune->pitchRateKd->text()) // 6
            .arg(m_autotune->yawRateKp->text()) // 7
            .arg(m_autotune->yawRateKi->text()) // 8
            .arg(m_autotune->yawRateKd->text()) // 9
            .arg(m_autotune->lblOuterKp->text()) // 10
            .arg(m_autotune->lblOuterKi->text()) // 11
            .arg(m_autotune->derivativeCutoff->text()); // 12

    return message0 + message1 + message2;
}

/**
 * @brief ConfigAutotuneWidget::generateResultsJson
 * @return QJsonDocument containing autotune result data
 */
QJsonDocument ConfigAutotuneWidget::getResultsJson()
{
    deviceDescriptorStruct firmware;
    utilMngr->getBoardDescriptionStruct(firmware);

    QJsonObject rawSettings;

    QJsonObject json;
    json["dataVersion"] = 3;
    json["uniqueId"] = QString(QCryptographicHash::hash(utilMngr->getBoardCPUSerial(), QCryptographicHash::Sha256).toHex());

    QJsonObject vehicle, fw;
    fw["board"] = autotuneShareForm->getBoardType();
    fw["tag"] = firmware.gitTag;
    fw["commit"] = firmware.gitHash;
    QDateTime fwDate = QDateTime::fromString(firmware.gitDate, "yyyyMMdd hh:mm");
    fwDate.setTimeSpec(Qt::UTC); // this makes it append a Z to the string indicating UTC
    fw["date"] = fwDate.toString(Qt::ISODate);
    vehicle["firmware"] = fw;

    SystemSettings *sysSettings = SystemSettings::GetInstance(getObjectManager());
    UAVObjectField *afTypeField = sysSettings->getField("AirframeType");
    QString afType;
    if(afTypeField) {
        QStringList vehicleTypes = afTypeField->getOptions();
        afType = vehicleTypes[sysSettings->getAirframeType()];
    }

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
        UAVObjectUtilManager* uavoUtilManager = pm->getObject<UAVObjectUtilManager>();
        Core::IBoardType* board = uavoUtilManager->getBoardType();
        if (board != NULL) {
            QString hwSettingsName = board->getHwUAVO();

            UAVObject *hwSettings = getObjectManager()->getObject(hwSettingsName);
            rawSettings[hwSettings->getName()] = hwSettings->getJsonRepresentation();
        }
    }

    vehicle["type"] = afType;
    vehicle["size"] = autotuneShareForm->getVehicleSize();
    vehicle["weight"] = autotuneShareForm->getWeight();
    vehicle["batteryCells"] = autotuneShareForm->getBatteryCells();
    vehicle["esc"] = autotuneShareForm->getESCs();
    vehicle["motor"] = autotuneShareForm->getMotors();
    vehicle["props"] = autotuneShareForm->getProps();
    json["vehicle"] = vehicle;

    json["userObservations"] = autotuneShareForm->getObservations();

    QJsonObject identification;
    // this stuff should be stored in an array so we can iterate :/
    QJsonObject roll_ident;
    roll_ident["gain"] = m_autotune->measuredRollGain->text().toDouble();
    roll_ident["bias"] = m_autotune->measuredRollBias->text().toDouble();
    roll_ident["noise"] = m_autotune->measuredRollNoise->text().toDouble();
    identification["roll"] = roll_ident;
    QJsonObject pitch_ident;
    pitch_ident["gain"] = m_autotune->measuredPitchGain->text().toDouble();
    pitch_ident["bias"] = m_autotune->measuredPitchBias->text().toDouble();
    pitch_ident["noise"] = m_autotune->measuredPitchNoise->text().toDouble();
    identification["pitch"] = pitch_ident;
    QJsonObject yaw_ident;
    yaw_ident["gain"] = m_autotune->measuredYawGain->text().toDouble();
    yaw_ident["bias"] = m_autotune->measuredYawBias->text().toDouble();
    yaw_ident["noise"] = m_autotune->measuredYawNoise->text().toDouble();
    identification["yaw"] = yaw_ident;

    identification["tau"] = m_autotune->rollTau->text().toDouble();
    json["identification"] = identification;

    QJsonObject tuning, parameters, computed, misc;
    parameters["damping"] = m_autotune->lblDamp->text().toDouble();

    QStringList noiseSens =  m_autotune->lblNoise->text().split(" ");

    if (!noiseSens.isEmpty()) {
        parameters["noiseSensitivity"] = noiseSens.first().toDouble();
    }

    tuning["parameters"] = parameters;
    computed["naturalFrequency"] = m_autotune->wn->text().toDouble();
    computed["derivativeCutoff"] = m_autotune->derivativeCutoff->text().toDouble();
    computed["converged"] = converged;
    computed["iterations"] = iterations;

    QJsonObject gains;
    QJsonObject roll_gain, pitch_gain, yaw_gain, outer_gain;
    roll_gain["kp"] = m_autotune->rollRateKp->text().toDouble();
    roll_gain["ki"] = m_autotune->rollRateKi->text().toDouble();
    roll_gain["kd"] = m_autotune->rollRateKd->text().toDouble();
    gains["roll"] = roll_gain;
    pitch_gain["kp"] = m_autotune->pitchRateKp->text().toDouble();
    pitch_gain["ki"] = m_autotune->pitchRateKi->text().toDouble();
    pitch_gain["kd"] = m_autotune->pitchRateKd->text().toDouble();
    gains["pitch"] = pitch_gain;
    yaw_gain["kp"] = m_autotune->yawRateKp->text().toDouble();
    yaw_gain["ki"] = m_autotune->yawRateKi->text().toDouble();
    yaw_gain["kd"] = m_autotune->yawRateKd->text().toDouble();
    gains["yaw"] = yaw_gain;
    outer_gain["kp"] = m_autotune->lblOuterKp->text().toDouble();
    outer_gain["ki"] = m_autotune->lblOuterKi->text().toDouble();
    gains["outer"] = outer_gain;
    computed["gains"] = gains;
    tuning["computed"] = computed;
    json["tuning"] = tuning;

    json["rawSettings"] = rawSettings;

    return QJsonDocument(json);
}

void ConfigAutotuneWidget::resetSliders()
{
    m_autotune->rateDamp->setValue(110);
    m_autotune->rateNoise->setValue(10);
}

void ConfigAutotuneWidget::onYawTuneToggled(bool checked)
{
    StabilizationSettings *stabilizationSettings = StabilizationSettings::GetInstance(getObjectManager());
    Q_ASSERT(stabilizationSettings);
    if(!stabilizationSettings)
        return;
    stabSettings = stabilizationSettings->getData();

    // save previous settings when yaw tuning is enabled
    if (checked) {
        if(!m_autotune->yawRateKp->property("Backup").isValid()) {
            m_autotune->yawRateKp->setProperty("Backup", stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KP]);
            m_autotune->yawRateKi->setProperty("Backup", stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KI]);
            m_autotune->yawRateKd->setProperty("Backup", stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KD]);
            connect(stabilizationSettings, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(onStabSettingsUpdated(UAVObject*)));
        }
    }

    // now we need to compute the gains and update UI
    recomputeStabilization();

    // restore previous settings when yaw tuning is disabled
    if (!checked) {
        if(m_autotune->yawRateKp->property("Backup").isValid())
            stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KP] = m_autotune->yawRateKp->property("Backup").toDouble();
        if(m_autotune->yawRateKi->property("Backup").isValid())
            stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KI] = m_autotune->yawRateKi->property("Backup").toDouble();
        if(m_autotune->yawRateKd->property("Backup").isValid())
            stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KD] = m_autotune->yawRateKd->property("Backup").toDouble();
    }
}

void ConfigAutotuneWidget::onStabSettingsUpdated(UAVObject *obj)
{
    // set the previous settings to new UAVO values
    stabSettings = static_cast<StabilizationSettings *>(obj)->getData();
    m_autotune->yawRateKp->setProperty("Backup", stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KP]);
    m_autotune->yawRateKi->setProperty("Backup", stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KI]);
    m_autotune->yawRateKd->setProperty("Backup", stabSettings.YawRatePID[StabilizationSettings::YAWRATEPID_KD]);
}

void ConfigAutotuneWidget::setApplyEnabled(const bool enable)
{
    m_autotune->useComputedValues->setEnabled(enable);
    m_autotune->gbxComputed->setVisible(enable);
    m_autotune->gbxFailure->setVisible(!enable);
}
