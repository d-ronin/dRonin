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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */

#include "configmodulewidget.h"

#include <coreplugin/iboardtype.h>
#include <uavobjectutil/uavobjectutilmanager.h>

#include "airspeedsettings.h"
#include "flightbatterysettings.h"
#include "hottsettings.h"
#include "flightbatterystate.h"
#include "geofencesettings.h"
#include "modulesettings.h"
#include "vibrationanalysissettings.h"
#include "taskinfo.h"
#include "loggingsettings.h"
#include "rgbledsettings.h"

#include <QColorDialog>
#include <QDebug>
#include <QPointer>

ConfigModuleWidget::ConfigModuleWidget(QWidget *parent)
    : ConfigTaskWidget(parent)
{
    ui = new Ui::Modules();
    ui->setupUi(this);

    // override anything set in .ui file by Qt Designer
    ui->moduleTab->setCurrentIndex(0);

    connect(this, &ConfigTaskWidget::autoPilotConnected, this, &ConfigModuleWidget::recheckTabs);

    // Populate UAVO strings
    AirspeedSettings *airspeedSettings;
    airspeedSettings = AirspeedSettings::GetInstance(getObjectManager());

    // Don't allow these to be changed here, only in the respective tabs.
    ui->cbAutotune->setDisabled(true);
    ui->cbTxPid->setDisabled(true);

    // This will be un-hidden as required by boards, but keep the unformatted string for later
    ui->lblOnBoardLeds->setVisible(false);
    ui->lblOnBoardLeds->setProperty("format", QVariant::fromValue(ui->lblOnBoardLeds->text()));

    // Connect auto-cell detection logic
    connect(ui->gbAutoCellDetection, &QGroupBox::toggled, this,
            &ConfigModuleWidget::autoCellDetectionToggled);
    connect(ui->sbMaxCellVoltage, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &ConfigModuleWidget::maxCellVoltageChanged);

    // Connect the voltage and current checkboxes, such that the ADC pins are toggled and vice versa
    connect(ui->gb_measureVoltage, &QGroupBox::toggled, this,
            &ConfigModuleWidget::toggleBatteryMonitoringPin);
    connect(ui->gb_measureCurrent, &QGroupBox::toggled, this,
            &ConfigModuleWidget::toggleBatteryMonitoringPin);
    connect(ui->cbVoltagePin, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigModuleWidget::toggleBatteryMonitoringGb);
    connect(ui->cbCurrentPin, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigModuleWidget::toggleBatteryMonitoringGb);

    // connect the voltage ratio and factor boxes so they update each other when edited
    connect(ui->sb_voltageRatio, &QAbstractSpinBox::editingFinished, this,
            &ConfigModuleWidget::updateVoltageRatio);
    connect(ui->sb_voltageFactor, &QAbstractSpinBox::editingFinished, this,
            &ConfigModuleWidget::updateVoltageFactor);
    connect(FlightBatterySettings::GetInstance(getObjectManager()),
            &FlightBatterySettings::SensorCalibrationFactor_VoltageChanged, this,
            &ConfigModuleWidget::updateVoltageFactorFromUavo);

    // Connect Airspeed Settings
    connect(airspeedSettings, &UAVObject::objectUpdated, this,
            &ConfigModuleWidget::updateAirspeedGroupbox);
    connect(ui->gb_airspeedGPS, &QGroupBox::clicked, this,
            &ConfigModuleWidget::enableAirspeedTypeGPS);
    connect(ui->gb_airspeedPitot, &QGroupBox::clicked, this,
            &ConfigModuleWidget::enableAirspeedTypePitot);

    setupLedTab();

    connect(this, &ConfigTaskWidget::autoPilotConnected,
            this, &ConfigModuleWidget::updateAnnunciatorTab);
    updateAnnunciatorTab();

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
    setNotMandatory(RGBLEDSettings::NAME);
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
    // clear checkboxes in-case taskinfo object doesn't exist (it will look like the last connected
    // board's modules are still enabled)
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

    UAVObject *obj;

    obj = getObjectManager()->getObject(AirspeedSettings::NAME);
    connect(obj, QOverload<UAVObject *, bool>::of(&UAVObject::transactionCompleted), this,
            &ConfigModuleWidget::objectUpdated, Qt::UniqueConnection);
    obj->requestUpdate();

    obj = getObjectManager()->getObject(FlightBatterySettings::NAME);
    connect(obj, QOverload<UAVObject *, bool>::of(&UAVObject::transactionCompleted), this,
            &ConfigModuleWidget::objectUpdated, Qt::UniqueConnection);
    obj->requestUpdate();

    obj = getObjectManager()->getObject(HoTTSettings::NAME);
    connect(obj, QOverload<UAVObject *, bool>::of(&UAVObject::transactionCompleted), this,
            &ConfigModuleWidget::objectUpdated, Qt::UniqueConnection);

    TaskInfo *taskInfo = qobject_cast<TaskInfo *>(getObjectManager()->getObject(TaskInfo::NAME));
    if (taskInfo && taskInfo->getIsPresentOnHardware()) {
        connect(taskInfo, QOverload<UAVObject *, bool>::of(&UAVObject::transactionCompleted), obj,
                &UAVObject::requestUpdate, Qt::UniqueConnection);
        taskInfo->requestUpdate();
    } else {
        obj->requestUpdate();
    }

    obj = getObjectManager()->getObject(LoggingSettings::NAME);
    connect(obj, QOverload<UAVObject *, bool>::of(&UAVObject::transactionCompleted), this,
            &ConfigModuleWidget::objectUpdated, Qt::UniqueConnection);
    obj->requestUpdate();

    // This requires re-evaluation so that board connection doesn't re-enable
    // the fields. TODO: use new ConfigTaskWidget::setWidgetEnabled function
    ui->cbAutotune->setDisabled(true);
    ui->cbTxPid->setDisabled(true);
}

//! Enable appropriate tab when objects are updated
void ConfigModuleWidget::objectUpdated(UAVObject *obj, bool success)
{
    if (!obj)
        return;

    ModuleSettings *moduleSettings =
        qobject_cast<ModuleSettings *>(getObjectManager()->getObject(ModuleSettings::NAME));
    Q_ASSERT(moduleSettings);
    TaskInfo *taskInfo = qobject_cast<TaskInfo *>(getObjectManager()->getObject(TaskInfo::NAME));
    bool enableHott = (taskInfo && taskInfo->getIsPresentOnHardware())
        ? (taskInfo->getRunning_UAVOHoTTBridge() == TaskInfo::RUNNING_TRUE)
        : true;
    bool enableGps = (taskInfo && taskInfo->getIsPresentOnHardware())
        ? (taskInfo->getRunning_GPS() == TaskInfo::RUNNING_TRUE)
        : true;
    enableGps &= moduleSettings->getIsPresentOnHardware();

    enableGpsTab(enableGps);

    QString objName = obj->getName();
    if (objName.compare(AirspeedSettings::NAME) == 0) {
        enableAirspeedTab(success
                          && moduleSettings->getAdminState_Airspeed()
                              == ModuleSettings::ADMINSTATE_ENABLED);
    } else if (objName.compare(FlightBatterySettings::NAME) == 0) {
        enableBatteryTab(success
                         && moduleSettings->getAdminState_Battery()
                             == ModuleSettings::ADMINSTATE_ENABLED);
        refreshAdcNames();
    } else if (objName.compare(HoTTSettings::NAME) == 0) {
        enableHoTTTelemetryTab(success && enableHott);
    } else if (objName.compare(LoggingSettings::NAME) == 0) {
        enableLoggingTab(success
                         && moduleSettings->getAdminState_Logging()
                             == ModuleSettings::ADMINSTATE_ENABLED);
    }
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
    } else {
        ui->gb_airspeedPitot->setChecked(true);
    }
}

/**
 * @brief ConfigModuleWidget::toggleAirspeedType Toggle the airspeed sensor type based on checkbox
 * @param checked
 */
void ConfigModuleWidget::enableAirspeedTypeGPS(bool checked)
{
    if (checked) {
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
    if (checked) {
        AirspeedSettings *airspeedSettings;
        airspeedSettings = AirspeedSettings::GetInstance(getObjectManager());
        AirspeedSettings::DataFields airspeedSettingsData;
        airspeedSettingsData = airspeedSettings->getData();
        airspeedSettingsData.AirspeedSensorType =
            AirspeedSettings::AIRSPEEDSENSORTYPE_EAGLETREEAIRSPEEDV3;
        airspeedSettings->setData(airspeedSettingsData);
    }
}

//! Enable or disable the battery tab
void ConfigModuleWidget::enableBatteryTab(bool enabled)
{
    int idx = ui->moduleTab->indexOf(ui->tabBattery);
    ui->moduleTab->setTabEnabled(idx, enabled);
}

//! Enable or disable the airspeed tab
void ConfigModuleWidget::enableAirspeedTab(bool enabled)
{
    int idx = ui->moduleTab->indexOf(ui->tabAirspeed);
    ui->moduleTab->setTabEnabled(idx, enabled);
}

//! Enable or disable the HoTT telemetrie tab
void ConfigModuleWidget::enableHoTTTelemetryTab(bool enabled)
{
    int idx = ui->moduleTab->indexOf(ui->tabHoTTTelemetry);
    ui->moduleTab->setTabEnabled(idx, enabled);
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
            ui->sbMaxCellVoltage->setValue(
                ui->sbMaxCellVoltage->property("ValueBackup").toDouble());
        else
            ui->sbMaxCellVoltage->setValue(
                4.2); // TODO: set this to default UAVO val instead of hardcoding?
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

void ConfigModuleWidget::setupLedTab()
{
    QPointer<UAVObjectManager> objMgr = getObjectManager();
    if (!objMgr) // hope it can only be destructed in this thread
        return;

    auto obj = qobject_cast<UAVDataObject *>(objMgr->getObject("RGBLEDSettings"));
    if (!obj)
        return;

    connect(obj, &UAVObject::objectUpdated, this, &ConfigModuleWidget::ledTabUpdate);
    connect(ui->btnLEDDefaultColor, &QPushButton::clicked, this,
            &ConfigModuleWidget::ledTabSetColor);
    connect(ui->btnLEDRange1Begin, &QPushButton::clicked, this,
            &ConfigModuleWidget::ledTabSetColor);
    connect(ui->btnLEDRange1End, &QPushButton::clicked, this, &ConfigModuleWidget::ledTabSetColor);
    connect(obj,
            static_cast<void (UAVDataObject::*)(bool)>(&UAVDataObject::presentOnHardwareChanged),
            [=](bool enable) { tabEnable(ui->tabLED, enable); });
    tabEnable(ui->tabLED, obj->getIsPresentOnHardware());
}

void ConfigModuleWidget::ledTabUpdate(UAVObject *obj)
{
    auto rgb = qobject_cast<RGBLEDSettings *>(obj);
    if (!rgb) {
        Q_ASSERT(false);
        qWarning() << "Invalid object!";
        return;
    }

    auto data = rgb->getData();

    ui->lblLEDDefaultColor->setStyleSheet(QString("background: rgb(%0, %1, %2);")
                                              .arg(data.DefaultColor[0])
                                              .arg(data.DefaultColor[1])
                                              .arg(data.DefaultColor[2]));
    ui->lblLEDRange1Begin->setStyleSheet(QString("background: rgb(%0, %1, %2);")
                                             .arg(data.RangeBaseColor[0])
                                             .arg(data.RangeBaseColor[1])
                                             .arg(data.RangeBaseColor[2]));
    ui->lblLEDRange1End->setStyleSheet(QString("background: rgb(%0, %1, %2);")
                                           .arg(data.RangeEndColor[0])
                                           .arg(data.RangeEndColor[1])
                                           .arg(data.RangeEndColor[2]));
}

void ConfigModuleWidget::ledTabSetColor()
{
    QPointer<QPushButton> btn = qobject_cast<QPushButton *>(sender());
    if (!btn)
        return;

    QPointer<UAVObjectManager> objMgr = getObjectManager();
    if (!objMgr)
        return;

    auto obj = objMgr->getObject("RGBLEDSettings");
    if (!obj)
        return;
    auto field = obj->getField(btn->property("fieldname").toString());
    if (!field)
        return;

    const QColor initial(field->getValue(0).toInt(), field->getValue(1).toInt(),
                         field->getValue(2).toInt());
    QColorDialog picker(initial, this);
    picker.raise();
    if (picker.exec() != QDialog::Accepted)
        return;

    const QColor &col = picker.selectedColor();
    field->setValue(col.red(), 0);
    field->setValue(col.green(), 1);
    field->setValue(col.blue(), 2);

    QPointer<QLabel> lbl(ui->tabLED->findChild<QLabel *>(btn->objectName().replace("btn", "lbl")));
    if (!lbl)
        return;

    lbl->setStyleSheet(
        QString("background: rgb(%0, %1, %2);").arg(col.red()).arg(col.green()).arg(col.blue()));
}

void ConfigModuleWidget::tabEnable(QWidget *tab, bool enable)
{
    int idx = ui->moduleTab->indexOf(tab);
    if (ui->moduleTab->currentIndex() == idx)
        ui->moduleTab->setCurrentIndex(0);
    if (idx >= 0)
        ui->moduleTab->setTabEnabled(idx, enable);
}

/**
 * @brief Go through and show/hide relevant annunciator widgets for the current board
 */
void ConfigModuleWidget::updateAnnunciatorTab()
{
    struct AnuncType {
        Core::IBoardType::AnnunciatorType type;
        QString widgetName;
    };
    static const std::vector<AnuncType> annuncTypes = {
        { Core::IBoardType::ANNUNCIATOR_HEARTBEAT, "Heartbeat" },
        { Core::IBoardType::ANNUNCIATOR_ALARM, "Alarm" },
        { Core::IBoardType::ANNUNCIATOR_BUZZER, "Buzzer" },
        { Core::IBoardType::ANNUNCIATOR_RGB, "Rgb" },
        { Core::IBoardType::ANNUNCIATOR_DAC, "Dac" },
    };
    static const std::vector<QString> widgets = {
        "lbl%0Anytime",
        "cbx%0Anytime",
        "lbl%0Armed",
        "cbx%0Armed",
    };

    auto board = utilMngr->getBoardType();
    if (!board)
        return;

    for (const auto &annunc : annuncTypes) {
        bool have = board->hasAnnunciator(annunc.type);

        for (const auto &name : widgets) {
            const auto widget = ui->tabAnnunc->findChild<QWidget *>(name.arg(annunc.widgetName));
            if (widget)
                widget->setVisible(have);
            else
                qWarning() << "Failed to get widget:" << name.arg(annunc.widgetName);
        }
    }

    // tell the user if some of the LEDs are on the PCB
    const int onBoard = board->onBoardRgbLeds();
    ui->lblOnBoardLeds->setVisible(onBoard > 0);
    if (onBoard > 0) {
        ui->lblOnBoardLeds->setText(
                    ui->lblOnBoardLeds->property("format").toString().arg(onBoard));
    }
}

/**
 * @}
 * @}
 */
