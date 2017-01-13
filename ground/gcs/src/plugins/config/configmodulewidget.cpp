/**
 ******************************************************************************
 * @file       configmodulewidget.cpp
 * @brief      Configure the optional modules
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "configmodulewidget.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/generalsettings.h>
#include <coreplugin/iboardtype.h>


#include "airspeedsettings.h"
#include "flightbatterysettings.h"
#include "hottsettings.h"
#include "flightbatterystate.h"
#include "geofencesettings.h"
#include "modulesettings.h"
#include "vibrationanalysissettings.h"
#include "taskinfo.h"
#include "loggingsettings.h"

ConfigModuleWidget::ConfigModuleWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    ui = new Ui::Modules();
    ui->setupUi(this);

    connect(this, SIGNAL(autoPilotConnected()), this, SLOT(recheckTabs()));

    // Populate UAVO strings
    AirspeedSettings *airspeedSettings;
    airspeedSettings = AirspeedSettings::GetInstance(getObjectManager());

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Core::Internal::GeneralSettings *settings = pm->getObject<Core::Internal::GeneralSettings>();
    ui->saveRAM->setVisible(settings->useExpertMode());

    // Don't allow these to be changed here, only in the respective tabs.
    ui->cbAutotune->setDisabled(true);
    ui->cbTxPid->setDisabled(true);
    ui->cbCameraStab->setDisabled(true);

    // Connect auto-cell detection logic
    connect(ui->gbAutoCellDetection, SIGNAL(toggled(bool)), this, SLOT(autoCellDetectionToggled(bool)));
    connect(ui->sbMaxCellVoltage, SIGNAL(valueChanged(double)), this, SLOT(maxCellVoltageChanged(double)));

    // Connect the voltage and current checkboxes, such that the ADC pins are toggled and vice versa
    connect(ui->gb_measureVoltage, SIGNAL(toggled(bool)), this, SLOT(toggleBatteryMonitoringPin()));
    connect(ui->gb_measureCurrent, SIGNAL(toggled(bool)), this, SLOT(toggleBatteryMonitoringPin()));
    connect(ui->cbVoltagePin, SIGNAL(currentIndexChanged(int)), this, SLOT(toggleBatteryMonitoringGb()));
    connect(ui->cbCurrentPin, SIGNAL(currentIndexChanged(int)), this, SLOT(toggleBatteryMonitoringGb()));

    // connect the voltage ratio and factor boxes so they update each other when edited
    connect(ui->sb_voltageRatio, SIGNAL(editingFinished()), this, SLOT(updateVoltageRatio()));
    connect(ui->sb_voltageFactor, SIGNAL(editingFinished()), this, SLOT(updateVoltageFactor()));
    connect(FlightBatterySettings::GetInstance(getObjectManager()), SIGNAL(SensorCalibrationFactor_VoltageChanged(float)), this, SLOT(updateVoltageFactorFromUavo(float)));

    // Connect Airspeed Settings
    connect(airspeedSettings, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(updateAirspeedGroupbox(UAVObject *)));
    connect(ui->gb_airspeedGPS, SIGNAL(clicked(bool)), this, SLOT(enableAirspeedTypeGPS(bool)));
    connect(ui->gb_airspeedPitot, SIGNAL(clicked(bool)), this, SLOT(enableAirspeedTypePitot(bool)));

    enableBatteryTab(false);
    enableAirspeedTab(false);
    enableHoTTTelemetryTab(false);
    enableGpsTab(false);
    enableLoggingTab(false);

    // Load UAVObjects to widget relations from UI file
    // using objrelation dynamic property
    autoLoadWidgets();

    // Refresh widget contents
    refreshWidgetsValues();

    // Prevent mouse wheel from changing values
    disableMouseWheelEvents();

    setNotMandatory(FlightBatterySettings::NAME);
    setNotMandatory(AirspeedSettings::NAME);
    setNotMandatory(HoTTSettings::NAME);
    setNotMandatory(LoggingSettings::NAME);
}

ConfigModuleWidget::~ConfigModuleWidget()
{
    delete ui;
}

void ConfigModuleWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void ConfigModuleWidget::enableControls(bool enable)
{
    Q_UNUSED(enable);
}

//! Query optional objects to determine which tabs can be configured
void ConfigModuleWidget::recheckTabs()
{
    // clear checkboxes in-case taskinfo object doesn't exist (it will look like the last connected board's modules are still enabled)
    bool dirty = isDirty();
    ui->cbComBridge->setChecked(false);
    ui->cbGPS->setChecked(false);
    ui->cbUavoMavlink->setChecked(false);
    ui->cbUAVOHottBridge->setChecked(false);
    ui->cbUAVOLighttelemetryBridge->setChecked(false);
    ui->cbUAVOFrskyBridge->setChecked(false);
    ui->cbUAVOMSPBridge->setChecked(false);
    ui->cbUAVOFrSkySPortBridge->setChecked(false);
    setDirty(dirty);

    UAVObject * obj;

    obj = getObjectManager()->getObject(AirspeedSettings::NAME);
    connect(obj, SIGNAL(transactionCompleted(UAVObject*,bool)), this, SLOT(objectUpdated(UAVObject*,bool)), Qt::UniqueConnection);
    obj->requestUpdate();

    obj = getObjectManager()->getObject(FlightBatterySettings::NAME);
    connect(obj, SIGNAL(transactionCompleted(UAVObject*,bool)), this, SLOT(objectUpdated(UAVObject*,bool)), Qt::UniqueConnection);
    obj->requestUpdate();

    obj = getObjectManager()->getObject(HoTTSettings::NAME);
    connect(obj, SIGNAL(transactionCompleted(UAVObject*,bool)), this, SLOT(objectUpdated(UAVObject*,bool)), Qt::UniqueConnection);
    TaskInfo *taskInfo = qobject_cast<TaskInfo *>(getObjectManager()->getObject(TaskInfo::NAME));
    if (taskInfo && taskInfo->getIsPresentOnHardware()) {
        connect(taskInfo, SIGNAL(transactionCompleted(UAVObject *, bool)), obj, SLOT(requestUpdate()), Qt::UniqueConnection);
        taskInfo->requestUpdate();
    } else {
        obj->requestUpdate();
    }

    obj = getObjectManager()->getObject(LoggingSettings::NAME);
    connect(obj, SIGNAL(transactionCompleted(UAVObject*,bool)), this, SLOT(objectUpdated(UAVObject*,bool)), Qt::UniqueConnection);
    obj->requestUpdate();

    // This requires re-evaluation so that board connection doesn't re-enable
    // the fields. TODO: use new ConfigTaskWidget::setWidgetEnabled function
    ui->cbAutotune->setDisabled(true);
    ui->cbTxPid->setDisabled(true);
    ui->cbCameraStab->setDisabled(true);
}

//! Enable appropriate tab when objects are updated
void ConfigModuleWidget::objectUpdated(UAVObject * obj, bool success)
{
    if (!obj)
        return;

    ModuleSettings *moduleSettings = qobject_cast<ModuleSettings *>(getObjectManager()->getObject(ModuleSettings::NAME));
    Q_ASSERT(moduleSettings);
    TaskInfo *taskInfo = qobject_cast<TaskInfo *>(getObjectManager()->getObject(TaskInfo::NAME));
    bool enableHott = (taskInfo && taskInfo->getIsPresentOnHardware()) ? (taskInfo->getRunning_UAVOHoTTBridge() == TaskInfo::RUNNING_TRUE) : true;
    bool enableGps = (taskInfo && taskInfo->getIsPresentOnHardware()) ? (taskInfo->getRunning_GPS() == TaskInfo::RUNNING_TRUE) : true;
    enableGps &= moduleSettings->getIsPresentOnHardware();

    enableGpsTab(enableGps);

    QString objName = obj->getName();
    if (objName.compare(AirspeedSettings::NAME) == 0) {
        enableAirspeedTab(success && moduleSettings->getAdminState_Airspeed() == ModuleSettings::ADMINSTATE_ENABLED);
    } else if (objName.compare(FlightBatterySettings::NAME) == 0) {
        enableBatteryTab(success && moduleSettings->getAdminState_Battery() == ModuleSettings::ADMINSTATE_ENABLED);
        refreshAdcNames();
    } else if (objName.compare(HoTTSettings::NAME) == 0) {
        enableHoTTTelemetryTab(success && enableHott);
    } else if (objName.compare(LoggingSettings::NAME) == 0) {
        enableLoggingTab(success && moduleSettings->getAdminState_Logging() == ModuleSettings::ADMINSTATE_ENABLED);
    }

    // indicate that we can't tell which modules are running when taskinfo isn't available
    const bool haveTaskInfo = taskInfo->getIsPresentOnHardware();
    ui->lblAutoHaveTaskInfo->setVisible(haveTaskInfo);
    ui->lblAutoNoTaskInfo->setVisible(!haveTaskInfo);
    ui->gbAutoModules->setEnabled(haveTaskInfo);
}

/**
 * @brief Toggle voltage and current pins depending on battery monitoring checkboxes
 */
void ConfigModuleWidget::toggleBatteryMonitoringPin()
{
    if (!ui->gb_measureVoltage->isChecked())
        ui->cbVoltagePin->setCurrentIndex(ui->cbVoltagePin->findText("NONE"));

    if (!ui->gb_measureCurrent->isChecked())
        ui->cbCurrentPin->setCurrentIndex(ui->cbCurrentPin->findText("NONE"));
}

/**
 * @brief Toggle battery monitoring checkboxes depending on voltage and current pins
 */
void ConfigModuleWidget::toggleBatteryMonitoringGb()
{
    if (ui->cbVoltagePin->currentText().compare("NONE") != 0)
        ui->gb_measureVoltage->setChecked(true);
    else
        ui->gb_measureVoltage->setChecked(false);

    if (ui->cbCurrentPin->currentText().compare("NONE") != 0)
        ui->gb_measureCurrent->setChecked(true);
    else
        ui->gb_measureCurrent->setChecked(false);
}

/**
 * @brief ConfigModuleWidget::updateAirspeedGroupbox Updates groupbox when airspeed UAVO changes
 * @param obj
 */
void ConfigModuleWidget::updateAirspeedGroupbox(UAVObject *obj)
{
    Q_UNUSED(obj);

    AirspeedSettings *airspeedSettings;
    airspeedSettings = AirspeedSettings::GetInstance(getObjectManager());
    AirspeedSettings::DataFields airspeedSettingsData;
    airspeedSettingsData = airspeedSettings->getData();

    ui->gb_airspeedGPS->setChecked(false);
    ui->gb_airspeedPitot->setChecked(false);

    if (airspeedSettingsData.AirspeedSensorType == AirspeedSettings::AIRSPEEDSENSORTYPE_GPSONLY) {
        ui->gb_airspeedGPS->setChecked(true);
    }
    else {
         ui->gb_airspeedPitot->setChecked(true);
    }
}

/**
 * @brief ConfigModuleWidget::toggleAirspeedType Toggle the airspeed sensor type based on checkbox
 * @param checked
 */
void ConfigModuleWidget::enableAirspeedTypeGPS(bool checked)
{
    if (checked){
        AirspeedSettings *airspeedSettings;
        airspeedSettings = AirspeedSettings::GetInstance(getObjectManager());
        AirspeedSettings::DataFields airspeedSettingsData;
        airspeedSettingsData = airspeedSettings->getData();
        airspeedSettingsData.AirspeedSensorType = AirspeedSettings::AIRSPEEDSENSORTYPE_GPSONLY;
        airspeedSettings->setData(airspeedSettingsData);
    }

}

/**
 * @brief ConfigModuleWidget::toggleAirspeedType Toggle the airspeed sensor type based on checkbox
 * @param checked
 */
void ConfigModuleWidget::enableAirspeedTypePitot(bool checked)
{
    if (checked){
        AirspeedSettings *airspeedSettings;
        airspeedSettings = AirspeedSettings::GetInstance(getObjectManager());
        AirspeedSettings::DataFields airspeedSettingsData;
        airspeedSettingsData = airspeedSettings->getData();
        airspeedSettingsData.AirspeedSensorType = AirspeedSettings::AIRSPEEDSENSORTYPE_EAGLETREEAIRSPEEDV3;
        airspeedSettings->setData(airspeedSettingsData);
    }

}

//! Enable or disable the battery tab
void ConfigModuleWidget::enableBatteryTab(bool enabled)
{
    int idx = ui->moduleTab->indexOf(ui->tabBattery);
    ui->moduleTab->setTabEnabled(idx,enabled);
}

//! Enable or disable the airspeed tab
void ConfigModuleWidget::enableAirspeedTab(bool enabled)
{
    int idx = ui->moduleTab->indexOf(ui->tabAirspeed);
    ui->moduleTab->setTabEnabled(idx,enabled);
}

//! Enable or disable the HoTT telemetrie tab
void ConfigModuleWidget::enableHoTTTelemetryTab(bool enabled)
{
    int idx = ui->moduleTab->indexOf(ui->tabHoTTTelemetry);
    ui->moduleTab->setTabEnabled(idx,enabled);
}

//! Enable or disable the GPS tab
void ConfigModuleWidget::enableGpsTab(bool enabled)
{
    int idx = ui->moduleTab->indexOf(ui->tabGPS);
    ui->moduleTab->setTabEnabled(idx, enabled);
}

//! Enable or disable the Logging tab
void ConfigModuleWidget::enableLoggingTab(bool enabled)
{
    int idx = ui->moduleTab->indexOf(ui->tabLogging);
    ui->moduleTab->setTabEnabled(idx, enabled);
}

//! Replace ADC combobox names on battery tab with board specific ones
void ConfigModuleWidget::refreshAdcNames(void)
{
    QStringList names;
    Core::IBoardType *board = utilMngr->getBoardType();
    if (board)
        names = board->getAdcNames();
    if (names.isEmpty())
        return;

    for (int i = 0; i <= 8; i++) {
        QString name;
        if (i < names.length())
            name = names[i] + QString(" (ADC%1)").arg(i);
        else
            name = QString("N/A (ADC%1)").arg(i);

        if (i < ui->cbVoltagePin->count())
            ui->cbVoltagePin->setItemText(i, name);
        if (i < ui->cbCurrentPin->count())
            ui->cbCurrentPin->setItemText(i, name);
    }
}

void ConfigModuleWidget::updateVoltageRatio()
{
    ui->sb_voltageFactor->setValue(1000.0 / ui->sb_voltageRatio->value());
}

void ConfigModuleWidget::updateVoltageFactor()
{
    ui->sb_voltageRatio->setValue(1000.0 / ui->sb_voltageFactor->value());
}

void ConfigModuleWidget::updateVoltageFactorFromUavo(float value)
{
    ui->sb_voltageRatio->setValue(1000.0 / (double)value);
}

void ConfigModuleWidget::autoCellDetectionToggled(bool checked)
{
    ui->sbMaxCellVoltage->setEnabled(checked);
    ui->lblMaxCellVoltage->setEnabled(checked);
    ui->sb_numBatteryCells->setEnabled(!checked);
    ui->lblNumBatteryCells->setEnabled(!checked);
    if (checked) {
        if (ui->sbMaxCellVoltage->property("ValueBackup").isValid())
            ui->sbMaxCellVoltage->setValue(ui->sbMaxCellVoltage->property("ValueBackup").toDouble());
        else
            ui->sbMaxCellVoltage->setValue(4.2); // TODO: set this to default UAVO val instead of hardcoding?
    } else {
        ui->sbMaxCellVoltage->setProperty("ValueBackup", ui->sbMaxCellVoltage->value());
        ui->sbMaxCellVoltage->setValue(0.0);
    }
}

void ConfigModuleWidget::maxCellVoltageChanged(double value)
{
    if (value > 0.0) {
        if (!ui->gbAutoCellDetection->isChecked())
            ui->gbAutoCellDetection->setChecked(true);
    } else if (ui->gbAutoCellDetection->isChecked()) {
        ui->gbAutoCellDetection->setChecked(false);
    }
}

/**
 * @}
 * @}
 */
