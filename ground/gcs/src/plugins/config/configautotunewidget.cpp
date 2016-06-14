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
#define CONF_ATUNE_QXTLOG_DEBUG(...) qDebug()<<__VA_ARGS__
#else  // CONF_ATUNE_DEBUG
#define CONF_ATUNE_QXTLOG_DEBUG(...)
#endif	// CONF_ATUNE_DEBUG

ConfigAutotuneWidget::ConfigAutotuneWidget(ConfigGadgetWidget *parent) :
    ConfigTaskWidget(parent)
{
    parentConfigWidget = parent;
    m_autotune = new Ui_AutotuneWidget();
    m_autotune->setupUi(this);

    // Connect automatic signals
    autoLoadWidgets();
    disableMouseWheelEvents();

    addUAVObject(ModuleSettings::NAME);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    utilMngr = pm->getObject<UAVObjectUtilManager>();

    connect(this, SIGNAL(autoPilotConnected()), this, SLOT(checkNewAutotune()));
    connect(m_autotune->adjustTune, SIGNAL(pressed()), this, SLOT(openAutotuneDialog()));

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
        openAutotuneDialog();
    }
}

QString ConfigAutotuneWidget::systemIdentValid(SystemIdent::DataFields &data,
        bool *okToContinue)
{
    if (data.Tau == 0) {
        // Invalid / no tune.

        *okToContinue = false;
        return tr("<font color=\"crimson\">It doesn't appear an autotune was successfully completed and saved; we are unable to continue.</font>");
    }

    QString retVal;

    *okToContinue = true;

    if (data.NumAfPredicts < 5000) {
        // Wayyyyy too few points to be plausible.  CC3D is expected to do
        // 333 * 60 = 20000 give or take a bit, with most other platforms way
        // more.  If you don't get this many, it doesn't make sense.

        retVal.append(tr("Error: Too few measurements were obtained to successfully autotune this craft."));
        retVal.append("<br/>");
        *okToContinue = false;
    }

    if (data.NumSpilledPts > (data.NumAfPredicts / 8)) {
        // Greater than 12.5% points spilled / not processed.  You're not going
        // to have a good time.

        retVal.append(tr("Error: Many measurements were lost because the flight controller was unable to keep up with the tuning process."));
        retVal.append("<br/>");
        *okToContinue = false;
    } else if (data.NumSpilledPts > 10) {
        // We shouldn't spill points at all.  Warn!
        retVal.append(tr("Warning: Some measurements were lost because the flight controller was unable to keep up with the tuning process."));
        retVal.append("<br/>");
    }

    if (data.Tau < -5.3) {
        // Too low of tau to be plausible. (5ms)
        retVal.append(tr("Error: Autotune did not measure valid values for this craft (low tau).  Consider slightly lowering the starting roll/pitch rate P values or slightly decreasing Motor Input/Output Curve Fit on the output pane."));
        retVal.append("<br/>");
        *okToContinue = false;
    } else if (data.Tau < -4.9) {
        // Probably too low to be real-- 7.4ms-- warn!
        retVal.append(tr("Warning: The tau value measured for this craft is very low."));
        retVal.append("<br/>");
    } else if (data.Tau > -1.4) {
        // Too high of a tau to be plausible / accurate (247ms)-- warn!
        retVal.append(tr("Warning: The tau value measured for this craft is very high."));
        retVal.append("<br/>");
    }

    // Lowest valid gains seen have been 7.9, with most values in the range 9..11
    if (data.Beta[SystemIdent::BETA_ROLL] < 7.25) {
        retVal.append(tr("Error: Autotune did not measure valid values for this craft (low roll gain)."));
        retVal.append("<br/>");
        *okToContinue = false;
    }

    if (data.Beta[SystemIdent::BETA_PITCH] < 7.25) {
        retVal.append(tr("Error: Autotune did not measure valid values for this craft (low pitch gain)."));
        retVal.append("<br/>");
        *okToContinue = false;
    }

    retVal.replace(QRegExp("(\\w+:)"), "<font color=\"crimson\"><b>\\1</b></font>");

    if (*okToContinue) {
        if (retVal.isEmpty()) {
            retVal.append(tr("Everything checks out, and we're ready to proceed!"));
        } else {
            retVal.append(tr("<br/>These warnings may result in an invalid tune.  Proceed with caution."));
        }
    } else {
        retVal.append(tr("<br/>Unable to complete the autotune process because of the above error(s)."));
    }

    return retVal;
}

// XXX TODO change first screen text based on whether auto-opened
void ConfigAutotuneWidget::openAutotuneDialog(bool autoOpened)
{
    QWizard wizard;

    SystemIdent *systemIdent = SystemIdent::GetInstance(getObjectManager());
    SystemIdent::DataFields systemIdentData = systemIdent->getData();

    bool dataValid;
    QString initialWarnings = systemIdentValid(systemIdentData, &dataValid);

    // The first page is simple; construct it from scratch.
    QWizardPage *beginning = new QWizardPage;
    beginning->setTitle(tr("Examining autotune..."));

    if (autoOpened) {
        beginning->setSubTitle(tr("It looks like you have run a new autotune since you last connected to the flight controller.  This wizard will assist you in applying an autotune's measurement to your aircraft."));
    } else {
        beginning->setSubTitle(tr("This wizard will assist you in applying an autotune's measurement to your aircraft."));
    }

    QLabel *status = new QLabel(initialWarnings);
    status->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(status);
    beginning->setLayout(layout);

    // Keep a cancel button, even on OS X. 
    wizard.setOption(QWizard::NoCancelButton, false);

    wizard.setMinimumSize(688, 480);

    wizard.addPage(beginning);

    // Needed for sliders page to communicate with final page and end of wiz
    struct AutotunedValues av;

    if (dataValid) {
        wizard.addPage(new AutotuneMeasuredPropertiesPage(NULL, systemIdentData));
        wizard.addPage(new AutotuneSlidersPage(NULL, systemIdentData, &av));
        wizard.addPage(new AutotuneFinalPage(NULL, &av));
    }

    wizard.setWindowTitle("Autotune Wizard");
    wizard.exec();
}

AutotuneMeasuredPropertiesPage::AutotuneMeasuredPropertiesPage(QWidget *parent,
        SystemIdent::DataFields &systemIdentData) :
    QWizardPage(parent)
{
    setupUi(this);

    this->sysIdent = systemIdentData;
}

void AutotuneMeasuredPropertiesPage::initializePage()
{
    measuredRollGain->setText(
            QString::number(sysIdent.Beta[SystemIdent::BETA_ROLL], 'f', 2));
    measuredPitchGain->setText(
            QString::number(sysIdent.Beta[SystemIdent::BETA_PITCH], 'f', 2));
    measuredYawGain->setText(
            QString::number(sysIdent.Beta[SystemIdent::BETA_YAW], 'f', 2));

    measuredRollBias->setText(
            QString::number(sysIdent.Bias[SystemIdent::BIAS_ROLL], 'f', 3));
    measuredPitchBias->setText(
            QString::number(sysIdent.Bias[SystemIdent::BIAS_PITCH], 'f', 3));
    measuredYawBias->setText(
            QString::number(sysIdent.Bias[SystemIdent::BIAS_YAW], 'f', 3));

    double tau = exp(sysIdent.Tau);

    rollTau->setText(QString::number(tau, 'f', 4));
    pitchTau->setText(QString::number(tau, 'f', 4));

    measuredRollNoise->setText(
            QString::number(sysIdent.Noise[SystemIdent::NOISE_ROLL], 'f', 2));
    measuredPitchNoise->setText(
            QString::number(sysIdent.Noise[SystemIdent::NOISE_PITCH], 'f', 2));
    measuredYawNoise->setText(
            QString::number(sysIdent.Noise[SystemIdent::NOISE_YAW], 'f', 2));
}

AutotuneSlidersPage::AutotuneSlidersPage(QWidget *parent,
        SystemIdent::DataFields &systemIdentData,
        struct AutotunedValues *autoValues) :
    QWizardPage(parent)
{
    setupUi(this);

    sysIdent = systemIdentData;
    av = autoValues;

    // XXX disable yaw box based on observed beta
 
    // connect sliders to computation
    connect(rateDamp, SIGNAL(valueChanged(int)), this, SLOT(compute()));
    connect(rateNoise, SIGNAL(valueChanged(int)), this, SLOT(compute()));
    connect(cbUseYaw, SIGNAL(toggled(bool)), this, SLOT(compute()));
    connect(cbUseOuterKi, SIGNAL(toggled(bool)), this, SLOT(compute()));

    compute();
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

    bool doYaw = cbUseYaw->isChecked();
    bool doOuterKi = cbUseOuterKi->isChecked();

    double tau = exp(sysIdent.Tau);
    double beta_roll = sysIdent.Beta[SystemIdent::BETA_ROLL];
    double beta_pitch = sysIdent.Beta[SystemIdent::BETA_PITCH];

    double wn = 1/tau, wn_last = 1/tau + 10;
    double tau_d = 0, tau_d_last = 1000;

    const int iteration_limit = 100, stability_limit = 5;
    bool converged = false;
    int iterations = 0;
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

    av->converged = converged;

    if (!converged) {
        return;
    }

    av->derivativeCutoff = 1 / (2*M_PI*tau_d);
    av->naturalFreq = wn / 2 / M_PI;

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
    av->outerKp = 1 / 4.0 / (zeta_o * zeta_o) / (1/wn);

    if (doOuterKi) {
        av->outerKi = 0.75 * av->outerKp / (2 * M_PI * tau * 10.0);
    } else {
        av->outerKi = 0;
    }

    for (int i = 0; i < 2; i++) {
        double beta = exp(sysIdent.Beta[i]);

        double ki;
        double kp;
        double kd;

        ki = a * b * wn * wn * tau * tau_d / beta;
        kp = tau * tau_d * ((a+b)*wn*wn + 2*a*b*damp*wn) / beta - ki*tau_d;
        kd = (tau * tau_d * (a*b + wn*wn + (a+b)*2*damp*wn) - 1) / beta - kp * tau_d;

        av->kp[i] = kp;
        av->ki[i] = ki;
        av->kd[i] = kd;
    }

    if (doYaw) {
        double scale = exp(0.6 * (sysIdent.Beta[0] - sysIdent.Beta[2]));
        av->kp[2] = av->kp[0] * scale;
        av->ki[2] = 0.8 * av->ki[0] * scale;
        av->kd[2] = 0.8 * av->kd[0] * scale;
    } else {
        av->kp[2] = -1;  av->ki[2] = -1;  av->kd[2] = -1;
    }

    // XXX TODO -- relo somewhere nicer
    // XXX TODO -- and handle non-convergence case
    // XXX TODO -- and whether able to continue
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

AutotuneFinalPage::AutotuneFinalPage(QWidget *parent,
        struct AutotunedValues *autoValues) :
    QWizardPage(parent)
{
    setupUi(this);

    av = autoValues;
}
