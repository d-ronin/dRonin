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

    if (dataValid) {
        wizard.addPage(new AutotuneMeasuredPropertiesPage(NULL, systemIdentData));
        wizard.addPage(new AutotuneSlidersPage(NULL));
        wizard.addPage(new AutotuneFinalPage(NULL));
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

AutotuneSlidersPage::AutotuneSlidersPage(QWidget *parent) :
    QWizardPage(parent)
{
    setupUi(this);
}

AutotuneFinalPage::AutotuneFinalPage(QWidget *parent) :
    QWizardPage(parent)
{
    setupUi(this);
}
