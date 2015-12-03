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
#include "systemident.h"
#include "stabilizationsettings.h"
#include "modulesettings.h"
#include "systemsettings.h"
#include "coreplugin/generalsettings.h"
#include "extensionsystem/pluginmanager.h"
#include "coreplugin/iboardtype.h"


const QString ConfigAutotuneWidget::databaseUrl = QString("http://dronin-autotown.appspot.com/storeTune");


ConfigAutotuneWidget::ConfigAutotuneWidget(QWidget *parent) :
    ConfigTaskWidget(parent)
{
    m_autotune = new Ui_AutotuneWidget();
    m_autotune->setupUi(this);

    // Connect automatic signals
    autoLoadWidgets();
    disableMouseWheelEvents();

    // Whenever any value changes compute new potential stabilization settings
    connect(m_autotune->rateDamp, SIGNAL(valueChanged(int)), this, SLOT(recomputeStabilization()));
    connect(m_autotune->rateNoise, SIGNAL(valueChanged(int)), this, SLOT(recomputeStabilization()));

    addUAVObject(ModuleSettings::NAME);

    SystemIdent *systemIdent = SystemIdent::GetInstance(getObjectManager());

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    utilMngr = pm->getObject<UAVObjectUtilManager>();

    addWidget(m_autotune->enableAutoTune);

    connect(systemIdent, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(recomputeStabilization()));

    // Connect the apply button for the stabilization settings
    connect(m_autotune->useComputedValues, SIGNAL(pressed()), this, SLOT(saveStabilization()));

    connect(m_autotune->shareDataPB, SIGNAL(pressed()),this, SLOT(onShareData()));

    setNotMandatory(systemIdent->getName());
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
    if (approveSettings(systemIdent->getData()) == false)
        return;

    // Make sure to recompute in case the other stab settings changed since
    // the last time
    recomputeStabilization();

    // Apply this data to the board
    stabilizationSettings->setData(stabSettings);
    stabilizationSettings->updated();
}

void ConfigAutotuneWidget::onShareData()
{
    autotuneShareForm = new AutotuneShareForm(this);
    connect(autotuneShareForm, SIGNAL(finished(int)), this, SLOT(onShareFinished(int)));
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

void ConfigAutotuneWidget::onShareFinished(int value)
{
    Q_UNUSED(value);
    autotuneShareForm->deleteLater();
}

void ConfigAutotuneWidget::onShareToDatabase()
{
    autotuneShareForm->disableDatabase(true);
    // save data for next time the form is used
    saveUserData();

    autotuneShareForm->disableProgress(false);
    autotuneShareForm->setProgress(20);

    QJsonDocument json = getResultsJson();

    QUrl url(databaseUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    connect(manager, SIGNAL(finished(QNetworkReply*)),
                this, SLOT(onShareToDatabaseComplete(QNetworkReply*)));

    manager->post(request, json.toJson());
}

void ConfigAutotuneWidget::onShareToDatabaseComplete(QNetworkReply *reply)
{
    if(reply->error() != QNetworkReply::NoError) {
        qWarning() << "[ConfigAutotuneWidget::onShareToDatabaseComplete]HTTP Error: " << reply->errorString();
        QMessageBox msgBox;
        msgBox.setText("An error occured!");
        msgBox.setInformativeText("Your results could not be shared to the database. Please try again later.");
        msgBox.setDetailedText(QString("URL: %1\nReply: %2\n")
                               .arg(reply->url().toString())
                               .arg(reply->errorString()));
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        autotuneShareForm->setProgress(0);
        autotuneShareForm->disableDatabase(false);
    }
    else {
        autotuneShareForm->setProgress(100);
        // database share button remains disabled, no need to send twice
    }
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

    double wn = 1/tau;
    double tau_d = 0;
    for (int i = 0; i < 30; i++) {
        double tau_d_roll = (2*damp*tau*wn - 1)/(4*tau*damp*damp*wn*wn - 2*damp*wn - tau*wn*wn + exp(beta_roll)*ghf);
        double tau_d_pitch = (2*damp*tau*wn - 1)/(4*tau*damp*damp*wn*wn - 2*damp*wn - tau*wn*wn + exp(beta_pitch)*ghf);

        // Select the slowest filter property
        tau_d = (tau_d_roll > tau_d_pitch) ? tau_d_roll : tau_d_pitch;
        wn = (tau + tau_d) / (tau*tau_d) / (2 * damp + 2);
    }

    // Set the real pole position. The first pole is quite slow, which
    // prevents the integral being too snappy and driving too much
    // overshoot.
    const double a = ((tau+tau_d) / tau / tau_d - 2 * damp * wn) / 20.0;
    const double b = ((tau+tau_d) / tau / tau_d - 2 * damp * wn - a);

    qDebug() << "ghf: " << ghf;
    qDebug() << "wn: " << wn << "tau_d: " << tau_d;
    qDebug() << "a: " << a << " b: " << b;

    // Calculate the gain for the outer loop by approximating the
    // inner loop as a single order lpf. Set the outer loop to be
    // critically damped;
    const double zeta_o = 1.3;
    const double kp_o = 1 / 4.0 / (zeta_o * zeta_o) / (1/wn);

    // For now just run over roll and pitch
    for (int i = 0; i < 2; i++) {
        double beta = exp(systemIdentData.Beta[i]);

        double ki = a * b * wn * wn * tau * tau_d / beta;
        double kp = tau * tau_d * ((a+b)*wn*wn + 2*a*b*damp*wn) / beta - ki*tau_d;
        double kd = (tau * tau_d * (a*b + wn*wn + (a+b)*2*damp*wn) - 1) / beta - kp * tau_d;

        switch(i) {
        case 0: // Roll
            stabSettings.RollRatePID[StabilizationSettings::ROLLRATEPID_KP] = kp;
            stabSettings.RollRatePID[StabilizationSettings::ROLLRATEPID_KI] = ki;
            stabSettings.RollRatePID[StabilizationSettings::ROLLRATEPID_KD] = kd;
            stabSettings.RollPI[StabilizationSettings::ROLLPI_KP] = kp_o;
            stabSettings.RollPI[StabilizationSettings::ROLLPI_KI] = 0;
            break;
        case 1: // Pitch
            stabSettings.PitchRatePID[StabilizationSettings::PITCHRATEPID_KP] = kp;
            stabSettings.PitchRatePID[StabilizationSettings::PITCHRATEPID_KI] = ki;
            stabSettings.PitchRatePID[StabilizationSettings::PITCHRATEPID_KD] = kd;
            stabSettings.PitchPI[StabilizationSettings::PITCHPI_KP] = kp_o;
            stabSettings.PitchPI[StabilizationSettings::PITCHPI_KI] = 0;
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
    m_autotune->lblOuterKp->setText(QString::number(stabSettings.RollPI[StabilizationSettings::ROLLPI_KP]));

    m_autotune->derivativeCutoff->setText(QString::number(stabSettings.DerivativeCutoff));
    m_autotune->rollTau->setText(QString::number(tau,'g',3));
    m_autotune->pitchTau->setText(QString::number(tau,'g',3));
    m_autotune->wn->setText(QString::number(wn / 2 / M_PI, 'f', 1));
    m_autotune->lblDamp->setText(QString::number(damp, 'g', 2));
    m_autotune->lblNoise->setText(QString::number(ghf * 100, 'g', 2) + " %");

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
    Core::IBoardType* board = utilMngr->getBoardType();
    QString boardName;
    if (board)
        boardName = board->shortName();

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
                "Aircraft description:\n"
                "Type:\t\t\t%4\n"
                "Weight (AUW):\t%5 g\n"
                "Size:\t\t\t%6\n"
                "Battery:\t\t%7S\n"
                "Motors:\t\t\t%8\n"
                "ESCs:\t\t\t%9\n\n"
                "Observations:\n%10\n\n")
            .arg(boardName) // 0
            .arg(firmware.gitTag) // 1
            .arg(firmware.gitHash.left(7)) // 2
            .arg(firmware.gitDate + " UTC") // 3
            .arg(afType) // 4
            .arg(autotuneShareForm->getWeight()) // 5
            .arg(autotuneShareForm->getVehicleSize()) // 6
            .arg(autotuneShareForm->getBatteryCells()) // 7
            .arg(autotuneShareForm->getMotors()) // 8
            .arg(autotuneShareForm->getESCs()) // 9
            .arg(autotuneShareForm->getObservations()); // 10
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
                "\t\t\tRateKp\t\tRateKi\t\tRateKd\n"
                "Roll:\t\t%0\t%1\t%2\n"
                "Pitch:\t\t%3\t%4\t%5\n"
                "Outer Kp:\t%6\t\t-\t\t\t-\n"
                "Derivative cutoff:\t%7")
            .arg(m_autotune->rollRateKp->text()) // 0
            .arg(m_autotune->rollRateKi->text()) // 1
            .arg(m_autotune->rollRateKd->text()) // 2
            .arg(m_autotune->pitchRateKp->text()) // 3
            .arg(m_autotune->pitchRateKi->text()) // 4
            .arg(m_autotune->pitchRateKd->text()) // 5
            .arg(m_autotune->lblOuterKp->text()) // 6
            .arg(m_autotune->derivativeCutoff->text()); // 7

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
    Core::IBoardType* board = utilMngr->getBoardType();
    QString boardName;
    if (board)
        boardName = board->shortName();

    QJsonObject json;
    json["dataVersion"] = 1;
    json["uniqueId"] = QString(utilMngr->getBoardCPUSerial().toHex());

    QJsonObject vehicle, fw;
    fw["board"] = boardName;
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
    vehicle["type"] = afType;
    vehicle["size"] = autotuneShareForm->getVehicleSize();
    vehicle["weight"] = autotuneShareForm->getWeight();
    vehicle["batteryCells"] = autotuneShareForm->getBatteryCells();
    vehicle["esc"] = autotuneShareForm->getESCs();
    vehicle["motor"] = autotuneShareForm->getMotors();
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
    identification["tau"] = m_autotune->rollTau->text().toDouble();
    json["identification"] = identification;

    QJsonObject tuning, parameters, computed;
    parameters["damping"] = m_autotune->lblDamp->text().toDouble();
    parameters["noiseSensitivity"] = m_autotune->lblNoise->text().toDouble();
    tuning["parameters"] = parameters;
    computed["naturalFrequency"] = m_autotune->wn->text().toDouble();
    computed["derivativeCutoff"] = m_autotune->derivativeCutoff->text().toDouble();
    QJsonObject gains;
    QJsonObject roll_gain, pitch_gain, outer_gain;
    roll_gain["kp"] = m_autotune->rollRateKp->text().toDouble();
    roll_gain["ki"] = m_autotune->rollRateKi->text().toDouble();
    roll_gain["kd"] = m_autotune->rollRateKd->text().toDouble();
    gains["roll"] = roll_gain;
    pitch_gain["kp"] = m_autotune->pitchRateKp->text().toDouble();
    pitch_gain["ki"] = m_autotune->pitchRateKi->text().toDouble();
    pitch_gain["kd"] = m_autotune->pitchRateKd->text().toDouble();
    gains["pitch"] = pitch_gain;
    outer_gain["kp"] = m_autotune->lblOuterKp->text().toDouble();
    gains["outer"] = outer_gain;
    computed["gains"] = gains;
    tuning["computed"] = computed;
    json["tuning"] = tuning;

    return QJsonDocument(json);
}
