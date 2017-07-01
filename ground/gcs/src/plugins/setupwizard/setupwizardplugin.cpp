/**
 ******************************************************************************
 *
 * @file       setupwizardplugin.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @see        The GNU Public License (GPL) Version 3
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup SetupWizard Setup Wizard
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
#include "setupwizardplugin.h"

#include <QDebug>
#include <QtPlugin>
#include <QStringList>
#include <extensionsystem/pluginmanager.h>

#include <config/configplugin.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <QKeySequence>
#include <coreplugin/modemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <uploader/uploadergadgetwidget.h>

SetupWizardPlugin::SetupWizardPlugin()
    : wizardRunning(false)
{
}

SetupWizardPlugin::~SetupWizardPlugin()
{
}

bool SetupWizardPlugin::initialize(const QStringList &args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    // Add Menu entry
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::ActionContainer *ac = am->actionContainer(Core::Constants::M_TOOLS);

    Core::Command *cmd = am->registerAction(new QAction(this), "SetupWizardPlugin.ShowSetupWizard",
                                            QList<int>() << Core::Constants::C_GLOBAL_ID);
    cmd->action()->setText(tr("Vehicle Setup Wizard"));

    Core::ModeManager::instance()->addAction(cmd, 1);

    ac->menu()->addSeparator();
    ac->appendGroup("Wizard");
    ac->addAction(cmd, "Wizard");

    connect(cmd->action(), &QAction::triggered, this, &SetupWizardPlugin::showSetupWizard);
    return true;
}

void SetupWizardPlugin::extensionsInitialized()
{
    auto pm = ExtensionSystem::PluginManager::instance();
    auto config = pm->getObject<ConfigPlugin>();
    if (config)
        connect(config, &ConfigPlugin::launchSetupWizard,
                std::bind(&SetupWizardPlugin::showSetupWizard, this, true));
}

void SetupWizardPlugin::shutdown()
{
}

void SetupWizardPlugin::showSetupWizard(bool autoLaunched)
{
    if (wizardRunning)
        return;

    if (autoLaunched) {
        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();

        // check if uploader is working with this board
        auto uploaders = pm->getObjects<uploader::UploaderGadgetWidget>();
        for (const auto uploader : uploaders) {
            if (uploader->active())
                return;
        }

        // check if it's been ignored already
        UAVObjectUtilManager *utilMngr = pm->getObject<UAVObjectUtilManager>();
        if (!utilMngr) {
            qWarning() << "Could not get UAVObjectUtilManager";
            Q_ASSERT(false);
            return;
        }
        if (ignoredBoards.contains(utilMngr->getBoardCPUSerial())) {
            qInfo() << "Ignoring board";
            return;
        }
    }

    Core::ModeManager::instance()->activateModeByWorkspaceName(Core::Constants::MODE_WELCOME);

    wizardRunning = true;
    SetupWizard *m_wiz = new SetupWizard(autoLaunched);
    connect(m_wiz, &SetupWizard::boardIgnored, this, &SetupWizardPlugin::ignoreBoard);
    connect(m_wiz, &QDialog::finished, this, &SetupWizardPlugin::wizardTerminated);
    m_wiz->setAttribute(Qt::WA_DeleteOnClose, true);
    m_wiz->setWindowFlags(m_wiz->windowFlags() | Qt::WindowStaysOnTopHint);
    m_wiz->show();
}

void SetupWizardPlugin::wizardTerminated()
{
    wizardRunning = false;
    disconnect(this, SLOT(wizardTerminated()));
}

void SetupWizardPlugin::ignoreBoard(QByteArray uuid)
{
    ignoredBoards << uuid;
}

/**
 * @}
 * @}
 */
