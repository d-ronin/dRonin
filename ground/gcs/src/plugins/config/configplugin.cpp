/**
 ******************************************************************************
 *
 * @file       configplugin.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to update settings in the firmware
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
#include "configplugin.h"
#include "configgadgetfactory.h"
#include <QtPlugin>
#include <QMainWindow>
#include <QStringList>
#include <QTimer>
#include <extensionsystem/pluginmanager.h>
#include "calibration.h"
#include "objectpersistence.h"

ConfigPlugin::ConfigPlugin()
{
    // Do nothing
}

ConfigPlugin::~ConfigPlugin()
{
    removeObject(this);
}

bool ConfigPlugin::initialize(const QStringList &args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);
    cf = new ConfigGadgetFactory(this);
    addAutoReleasedObject(cf);

    Calibration *cal = new Calibration();
    addAutoReleasedObject(cal);

    // Add Menu entry to erase all settings
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::ActionContainer *ac = am->actionContainer(Core::Constants::M_TOOLS);

    // Command to erase all settings from the board
    cmd = am->registerAction(new QAction(this), "ConfigPlugin.EraseAll",
                             QList<int>() << Core::Constants::C_GLOBAL_ID);
    cmd->action()->setText(tr("Erase all settings from board..."));

    ac->menu()->addSeparator();
    ac->appendGroup("Utilities");
    ac->addAction(cmd, "Utilities");

    connect(cmd->action(), &QAction::triggered, this, &ConfigPlugin::eraseAllSettings);

    // *********************
    // Listen to autopilot connection events
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    TelemetryManager *telMngr = pm->getObject<TelemetryManager>();
    connect(telMngr, &TelemetryManager::connected, this, &ConfigPlugin::onAutopilotConnect);
    connect(telMngr, &TelemetryManager::disconnected, this, &ConfigPlugin::onAutopilotDisconnect);

    cmd->action()->setEnabled(false);

    addObject(this);

    return true;
}

/**
 * @brief Return handle to object manager
 */
UAVObjectManager *ConfigPlugin::getObjectManager()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objMngr = pm->getObject<UAVObjectManager>();
    Q_ASSERT(objMngr);
    return objMngr;
}

void ConfigPlugin::extensionsInitialized() {}

void ConfigPlugin::shutdown()
{
    // Do nothing
}

/**
 * Enable the menu entry when the autopilot connects
 */
void ConfigPlugin::onAutopilotConnect()
{
    // erase action
    cmd->action()->setEnabled(true);

    auto pm = ExtensionSystem::PluginManager::instance();
    auto uavoUtilManager = pm->getObject<UAVObjectUtilManager>();

    if (uavoUtilManager->firmwareHashMatchesGcs()) {
        // check if the board has been configured for flight
        // if not, let's pop setup wizard automatically
        if (!uavoUtilManager->boardConfigured())
            Q_EMIT launchSetupWizard();
    } else {
        /** @todo upgrade firmware
         * Currently the uploader plugin does this indepenently
         * but it needs a big refactor because everything is done
         * inside a UAVGadget */
    }
}

/**
 * Enable the menu entry when the autopilot connects
 */
void ConfigPlugin::onAutopilotDisconnect()
{
    cmd->action()->setEnabled(false);
}

/**
 * Erase all settings from the board
 */
void ConfigPlugin::eraseAllSettings()
{
    QMessageBox msgBox(dynamic_cast<QWidget *>(Core::ICore::instance()->mainWindow()));
    msgBox.setText(tr("Are you sure you want to erase all board settings?."));
    msgBox.setInformativeText(tr("All settings stored in your board flash will be deleted."));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    if (msgBox.exec() != QMessageBox::Ok)
        return;

    settingsErased = false;

    // TODO: Replace the second and third [in eraseDone()] pop-up dialogs with a progress indicator,
    // counter, or infinite chain of `......` tied to the original dialog box
    msgBox.setText(tr("Settings will now erase."));
    msgBox.setInformativeText(tr("Press <OK> and then please wait until a completion box appears. "
                                 "This can take up to %1 seconds.")
                                  .arg(FLASH_ERASE_TIMEOUT_MS / 1000));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();

    ObjectPersistence *objper = ObjectPersistence::GetInstance(getObjectManager());
    Q_ASSERT(objper);

    connect(objper, &UAVObject::objectUpdated, this, &ConfigPlugin::eraseDone);

    ObjectPersistence::DataFields data = objper->getData();
    data.Operation = ObjectPersistence::OPERATION_FULLERASE;

    // No need for manual updated event, this is triggered by setData
    // based on UAVO meta data
    objper->setData(data);
    objper->updated();
    QTimer::singleShot(FLASH_ERASE_TIMEOUT_MS, this, &ConfigPlugin::eraseFailed);
}

void ConfigPlugin::eraseFailed()
{
    if (settingsErased)
        return;

    ObjectPersistence *objper = ObjectPersistence::GetInstance(getObjectManager());

    disconnect(objper, &UAVObject::objectUpdated, this, &ConfigPlugin::eraseDone);

    (void)QMessageBox::critical(
        dynamic_cast<QWidget *>(Core::ICore::instance()->mainWindow()),
        tr("Error erasing settings"),
        tr("Power-cycle your board after removing all blades. Settings might be inconsistent."),
        QMessageBox::Ok);
}

void ConfigPlugin::eraseDone(UAVObject *obj)
{
    Q_UNUSED(obj)
    QMessageBox msgBox(dynamic_cast<QWidget *>(Core::ICore::instance()->mainWindow()));
    ObjectPersistence *objper = ObjectPersistence::GetInstance(getObjectManager());
    ObjectPersistence::DataFields data = objper->getData();
    Q_ASSERT(obj->getInstID() == objper->getInstID());

    if (data.Operation != ObjectPersistence::OPERATION_COMPLETED) {
        return;
    }

    disconnect(objper, &UAVObject::objectUpdated, this, &ConfigPlugin::eraseDone);
    if (data.Operation == ObjectPersistence::OPERATION_COMPLETED) {
        settingsErased = true;
        msgBox.setText(tr("Settings are now erased."));
        msgBox.setInformativeText(tr("Please ensure that the status LED is flashing regularly and "
                                     "then power-cycle your board."));
    } else {
        msgBox.setText(tr("Error trying to erase settings."));
        msgBox.setInformativeText(tr(
            "Power-cycle your board after removing all blades. Settings might be inconsistent."));
    }
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}
