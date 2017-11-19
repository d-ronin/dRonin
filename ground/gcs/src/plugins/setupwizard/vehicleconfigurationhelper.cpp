/**
 ******************************************************************************
 *
 * @file       vehicleconfigurationhelper.cpp
 * @brief      Provide an interface between the settings selected and the wizard
 *             and storing them on the FC
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
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

#include "vehicleconfigurationhelper.h"

#include <actuatorsettings.h>
#include <attitudesettings.h>
#include <mixersettings.h>
#include <systemsettings.h>
#include <manualcontrolsettings.h>
#include <sensorsettings.h>
#include <stabilizationsettings.h>

#include <extensionsystem/pluginmanager.h>
#include <uavobjectutil/uavobjectutilmanager.h>

#include <QDebug>

const float VehicleConfigurationHelper::DEFAULT_ENABLED_ACCEL_TAU = 0.1;

VehicleConfigurationHelper::VehicleConfigurationHelper(VehicleConfigurationSource *configSource)
    : m_configSource(configSource)
    , m_uavoManager(0)
    , m_transactionOK(false)
    , m_transactionTimeout(false)
    , m_currentTransactionObjectID(-1)
    , m_progress(0)
{
    Q_ASSERT(m_configSource);
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    m_uavoManager = pm->getObject<UAVObjectManager>();
    Q_ASSERT(m_uavoManager);
}

bool VehicleConfigurationHelper::setupVehicle(bool save)
{
    m_progress = 0;
    clearModifiedObjects();
    resetVehicleConfig();
    resetGUIData();
    if (!saveChangesToController(save)) {
        return false;
    }

    m_progress = 0;
    applyHardwareConfiguration();
    applyVehicleConfiguration();
    applyActuatorConfiguration();
    applyFlighModeConfiguration();

    if (save) {
        applySensorBiasConfiguration();
    }

    applyStabilizationConfiguration();
    applyManualControlDefaults();

    bool result = saveChangesToController(save);
    emit saveProgress(m_modifiedObjects.count() + 1, ++m_progress,
                      result ? tr("Done!") : tr("Failed!"));
    return result;
}

bool VehicleConfigurationHelper::setupHardwareSettings(bool save)
{
    m_progress = 0;
    clearModifiedObjects();
    applyHardwareConfiguration();
    applyManualControlDefaults();

    bool result = saveChangesToController(save);
    emit saveProgress(m_modifiedObjects.count() + 1, ++m_progress,
                      result ? tr("Done!") : tr("Failed!"));
    return result;
}

void VehicleConfigurationHelper::addModifiedObject(UAVDataObject *object, QString description)
{
    m_modifiedObjects << new QPair<UAVDataObject *, QString>(object, description);
}

void VehicleConfigurationHelper::clearModifiedObjects()
{
    for (int i = 0; i < m_modifiedObjects.count(); i++) {
        QPair<UAVDataObject *, QString> *pair = m_modifiedObjects.at(i);
        delete pair;
    }
    m_modifiedObjects.clear();
}

/**
 * @brief VehicleConfigurationHelper::applyHardwareConfiguration Apply
 * settings to the board specific hardware settings via the board plugin
 *
 * The settings to apply were determined previously during the wizard.
 */
void VehicleConfigurationHelper::applyHardwareConfiguration()
{
    Core::IBoardType *boardPlugin = m_configSource->getControllerType();
    Q_ASSERT(boardPlugin);
    if (!boardPlugin)
        return;

    Core::IBoardType::InputType newType = m_configSource->getInputType();
    bool success = boardPlugin->setInputType(newType);

    if (success) {
        UAVDataObject *hwSettings =
            dynamic_cast<UAVDataObject *>(m_uavoManager->getObject(boardPlugin->getHwUAVO()));
        Q_ASSERT(hwSettings);
        if (hwSettings)
            addModifiedObject(hwSettings, tr("Writing hardware settings"));
    }
}

void VehicleConfigurationHelper::applyVehicleConfiguration()
{
    switch (m_configSource->getVehicleType()) {
    case VehicleConfigurationSource::VEHICLE_MULTI: {
        switch (m_configSource->getVehicleSubType()) {
        case VehicleConfigurationSource::MULTI_ROTOR_TRI_Y:
            setupTriCopter();
            break;
        case VehicleConfigurationSource::MULTI_ROTOR_QUAD_X:
        case VehicleConfigurationSource::MULTI_ROTOR_QUAD_PLUS:
            setupQuadCopter();
            break;
        case VehicleConfigurationSource::MULTI_ROTOR_HEXA:
        case VehicleConfigurationSource::MULTI_ROTOR_HEXA_COAX_Y:
        case VehicleConfigurationSource::MULTI_ROTOR_HEXA_H:
            setupHexaCopter();
            break;
        case VehicleConfigurationSource::MULTI_ROTOR_OCTO:
        case VehicleConfigurationSource::MULTI_ROTOR_OCTO_COAX_X:
        case VehicleConfigurationSource::MULTI_ROTOR_OCTO_COAX_PLUS:
        case VehicleConfigurationSource::MULTI_ROTOR_OCTO_V:
            setupOctoCopter();
            break;
        default:
            break;
        }
        break;
    }
    case VehicleConfigurationSource::VEHICLE_FIXEDWING:
    case VehicleConfigurationSource::VEHICLE_HELI:
    case VehicleConfigurationSource::VEHICLE_SURFACE:
        // TODO: Implement settings for other vehicle types?
        break;
    default:
        break;
    }
}

void VehicleConfigurationHelper::applyActuatorConfiguration()
{
    ActuatorSettings *actSettings = ActuatorSettings::GetInstance(m_uavoManager);

    Core::IBoardType *boardPlugin = m_configSource->getControllerType();
    Q_ASSERT(boardPlugin);
    if (!boardPlugin)
        return;

    switch (m_configSource->getVehicleType()) {
    case VehicleConfigurationSource::VEHICLE_MULTI: {
        ActuatorSettings::DataFields data = actSettings->getData();

        QList<actuatorChannelSettings> actuatorSettings = m_configSource->getActuatorSettings();
        data.MotorsSpinWhileArmed = ActuatorSettings::MOTORSSPINWHILEARMED_FALSE;

        for (quint16 i = 0; i < ActuatorSettings::TIMERUPDATEFREQ_NUMELEM; i++) {
            data.TimerUpdateFreq[i] = ESC_FREQUENCY_LEGACY;
        }

        int max_motors = 0;

        quint16 updateFrequency = ESC_FREQUENCY_LEGACY;
        switch (m_configSource->getESCType()) {
        case VehicleConfigurationSource::ESC_LEGACY:
            updateFrequency = ESC_FREQUENCY_LEGACY;
            break;
        case VehicleConfigurationSource::ESC_RAPID:
            updateFrequency = ESC_FREQUENCY_RAPID;
            break;
        case VehicleConfigurationSource::ESC_ONESHOT125:
        case VehicleConfigurationSource::ESC_ONESHOT42:
            updateFrequency = ESC_FREQUENCY_SYNCPWM;
            break;
        case VehicleConfigurationSource::ESC_DSHOT300:
            updateFrequency = ESC_FREQUENCY_DSHOT300;
            break;
        case VehicleConfigurationSource::ESC_DSHOT600:
            updateFrequency = ESC_FREQUENCY_DSHOT600;
            break;
        case VehicleConfigurationSource::ESC_DSHOT1200:
            updateFrequency = ESC_FREQUENCY_DSHOT1200;
            break;
        default:
            break;
        }

        // TOOD: vehicle specific sets of update frequencies
        switch (m_configSource->getVehicleSubType()) {
        case VehicleConfigurationSource::MULTI_ROTOR_TRI_Y:
            // Ignore the servo for now.
            max_motors = 3;

            data.MotorsSpinWhileArmed = ActuatorSettings::MOTORSSPINWHILEARMED_TRUE;
            break;
        case VehicleConfigurationSource::MULTI_ROTOR_QUAD_X:
        case VehicleConfigurationSource::MULTI_ROTOR_QUAD_PLUS:
            max_motors = 4;

            data.MotorsSpinWhileArmed = ActuatorSettings::MOTORSSPINWHILEARMED_TRUE;
            break;
        case VehicleConfigurationSource::MULTI_ROTOR_HEXA:
        case VehicleConfigurationSource::MULTI_ROTOR_HEXA_COAX_Y:
        case VehicleConfigurationSource::MULTI_ROTOR_HEXA_H:
            max_motors = 6;

            data.MotorsSpinWhileArmed = ActuatorSettings::MOTORSSPINWHILEARMED_TRUE;

            break;
        case VehicleConfigurationSource::MULTI_ROTOR_OCTO:
        case VehicleConfigurationSource::MULTI_ROTOR_OCTO_COAX_X:
        case VehicleConfigurationSource::MULTI_ROTOR_OCTO_COAX_PLUS:
        case VehicleConfigurationSource::MULTI_ROTOR_OCTO_V:
            max_motors = 8;

            data.MotorsSpinWhileArmed = ActuatorSettings::MOTORSSPINWHILEARMED_TRUE;
            break;
        default:
            break;
        }

        for (quint16 i = 0; i < ActuatorSettings::CHANNELMAX_NUMELEM; i++) {
            if (i < max_motors) {
                data.ChannelMin[i] = actuatorSettings[i].channelMin;
                data.ChannelNeutral[i] = actuatorSettings[i].channelNeutral;
                data.ChannelMax[i] = actuatorSettings[i].channelMax;

                // Channels are 1-indexed in boardplugin and UI. :(
                int bankNum = boardPlugin->getBankFromOutputChannel(i + 1);

                if (bankNum >= 0) {
                    data.TimerUpdateFreq[bankNum] = updateFrequency;
                }
            } else {
                data.ChannelMin[i] = 0;
                data.ChannelNeutral[i] = 0;
                data.ChannelMax[i] = 0;
            }
        }

        actSettings->setData(data);
        addModifiedObject(actSettings, tr("Writing actuator settings"));
        break;
    }
    case VehicleConfigurationSource::VEHICLE_FIXEDWING:
    case VehicleConfigurationSource::VEHICLE_HELI:
    case VehicleConfigurationSource::VEHICLE_SURFACE:
        // TODO: Implement settings for other vehicle types?
        break;
    default:
        break;
    }
}

void VehicleConfigurationHelper::applyFlighModeConfiguration()
{
    ManualControlSettings *controlSettings = ManualControlSettings::GetInstance(m_uavoManager);

    Q_ASSERT(controlSettings);

    ManualControlSettings::DataFields data = controlSettings->getData();
    data.Stabilization1Settings[0] = ManualControlSettings::STABILIZATION1SETTINGS_ATTITUDE;
    data.Stabilization1Settings[1] = ManualControlSettings::STABILIZATION1SETTINGS_ATTITUDE;
    data.Stabilization1Settings[2] = ManualControlSettings::STABILIZATION1SETTINGS_AXISLOCK;
    data.Stabilization2Settings[0] = ManualControlSettings::STABILIZATION2SETTINGS_ATTITUDE;
    data.Stabilization2Settings[1] = ManualControlSettings::STABILIZATION2SETTINGS_ATTITUDE;
    data.Stabilization2Settings[2] = ManualControlSettings::STABILIZATION2SETTINGS_RATE;
    data.Stabilization3Settings[0] = ManualControlSettings::STABILIZATION3SETTINGS_RATE;
    data.Stabilization3Settings[1] = ManualControlSettings::STABILIZATION3SETTINGS_RATE;
    data.Stabilization3Settings[2] = ManualControlSettings::STABILIZATION3SETTINGS_RATE;
    data.FlightModeNumber = 3;
    data.FlightModePosition[0] = ManualControlSettings::FLIGHTMODEPOSITION_LEVELING;
    data.FlightModePosition[1] = ManualControlSettings::FLIGHTMODEPOSITION_ACRO;
    data.FlightModePosition[2] = ManualControlSettings::FLIGHTMODEPOSITION_STABILIZED1;
    data.FlightModePosition[3] = ManualControlSettings::FLIGHTMODEPOSITION_ALTITUDEHOLD;
    data.FlightModePosition[4] = ManualControlSettings::FLIGHTMODEPOSITION_POSITIONHOLD;
    data.FlightModePosition[5] = ManualControlSettings::FLIGHTMODEPOSITION_MANUAL;
    controlSettings->setData(data);
    addModifiedObject(controlSettings, tr("Writing flight mode settings"));
}

/**
 * @brief VehicleConfigurationHelper::applySensorBiasConfiguration save
 * the sensor settings computed by the calibration object.
 */
void VehicleConfigurationHelper::applySensorBiasConfiguration()
{
    // add the sensor settings and attitude settings determined by the calibration
    // object. for consistency with the other methods later calibration should store
    // the relevant parameters and apply them here.

    AttitudeSettings *attitudeSettings = AttitudeSettings::GetInstance(m_uavoManager);
    Q_ASSERT(attitudeSettings);
    if (attitudeSettings)
        addModifiedObject(attitudeSettings, tr("Writing board rotation settings"));

    SensorSettings *sensorSettings = SensorSettings::GetInstance(m_uavoManager);
    Q_ASSERT(sensorSettings);
    if (sensorSettings)
        addModifiedObject(sensorSettings, tr("Writing gyro bias"));
}

void VehicleConfigurationHelper::applyStabilizationConfiguration()
{
    StabilizationSettings *stabSettings = StabilizationSettings::GetInstance(m_uavoManager);
    Q_ASSERT(stabSettings);

    StabilizationSettings defaultSettings;
    stabSettings->setData(defaultSettings.getData());
    addModifiedObject(stabSettings, tr("Writing stabilization settings"));
}

void VehicleConfigurationHelper::applyMixerConfiguration(mixerChannelSettings channels[])
{
    // Set all mixer data
    MixerSettings *mSettings = MixerSettings::GetInstance(m_uavoManager);

    Q_ASSERT(mSettings);

    // Set Mixer types and values
    QString mixerTypePattern = "Mixer%1Type";
    QString mixerVectorPattern = "Mixer%1Vector";
    for (int i = 0; i < 10; i++) {
        UAVObjectField *field = mSettings->getField(mixerTypePattern.arg(i + 1));
        Q_ASSERT(field);
        field->setValue(field->getOptions().at(channels[i].type));

        field = mSettings->getField(mixerVectorPattern.arg(i + 1));
        Q_ASSERT(field);
        field->setValue((channels[i].throttle1 * 128) / 100, 0);
        field->setValue((channels[i].throttle2 * 128) / 100, 1);
        field->setValue((channels[i].roll * 128) / 100, 2);
        field->setValue((channels[i].pitch * 128) / 100, 3);
        field->setValue((channels[i].yaw * 128) / 100, 4);
        field->setValue(0, 5);          // Accessory0..Accessory2
        field->setValue(0, 6);
        field->setValue(0, 7);
    }

    // Apply updates
    mSettings->setData(mSettings->getData());
    addModifiedObject(mSettings, tr("Writing mixer settings"));
}

void VehicleConfigurationHelper::applyMultiGUISettings(SystemSettings::AirframeTypeOptions airframe,
                                                       GUIConfigDataUnion guiConfig)
{
    SystemSettings *sSettings = SystemSettings::GetInstance(m_uavoManager);

    Q_ASSERT(sSettings);
    SystemSettings::DataFields data = sSettings->getData();
    data.AirframeType = airframe;

    for (int i = 0; i < (int)(SystemSettings::AIRFRAMECATEGORYSPECIFICCONFIGURATION_NUMELEM); i++) {
        data.AirframeCategorySpecificConfiguration[i] = guiConfig.UAVObject[i];
    }

    sSettings->setData(data);
    addModifiedObject(sSettings, tr("Writing vehicle settings"));
}

void VehicleConfigurationHelper::applyManualControlDefaults()
{
    ManualControlSettings *mcSettings = ManualControlSettings::GetInstance(m_uavoManager);

    Q_ASSERT(mcSettings);
    ManualControlSettings::DataFields cData = mcSettings->getData();

    ManualControlSettings::ChannelGroupsOptions channelType =
        ManualControlSettings::CHANNELGROUPS_PWM;

    /* For all of these -- minVal and maxVal gathered from multiple autotune
     * configuration shares.  neutVal is the true midpoint.   throtNeutVal
     * is 4% up from min-- which is more than we usually do but conservative
     * for auto-setup.  There are exceptions / cases where this is not right,
     * like for openLRS s.bus.  But it's a decent start for people who will
     * enter their ranges by hand.
     */
    qint16 minVal = 1000;
    qint16 maxVal = 2000;

    switch (m_configSource->getInputType()) {
    case Core::IBoardType::INPUT_TYPE_PWM:
        channelType = ManualControlSettings::CHANNELGROUPS_PWM;
        break;
    case Core::IBoardType::INPUT_TYPE_PPM:
        channelType = ManualControlSettings::CHANNELGROUPS_PPM;
        break;
    case Core::IBoardType::INPUT_TYPE_SBUS:
    case Core::IBoardType::INPUT_TYPE_SBUSNONINVERTED:
        minVal = 172;
        maxVal = 1811;
        channelType = ManualControlSettings::CHANNELGROUPS_SBUS;
        break;
    case Core::IBoardType::INPUT_TYPE_DSM:
        // Sadly this is the worse one-- it depends upon resolution.
        // 74-950, 64-940 seen for 10 bit.  192-1856, 23-2025 for 11 bit.
        minVal = 70;
        maxVal = 940;
        channelType = ManualControlSettings::CHANNELGROUPS_DSM;
        break;
    case Core::IBoardType::INPUT_TYPE_HOTTSUMD:
    case Core::IBoardType::INPUT_TYPE_HOTTSUMH:
        minVal = 858;
        maxVal = 2138;
        channelType = ManualControlSettings::CHANNELGROUPS_HOTTSUM;
        break;
    case Core::IBoardType::INPUT_TYPE_IBUS:
        /* Seems to be 1000..2000 */
        channelType = ManualControlSettings::CHANNELGROUPS_IBUS;
        break;
    case Core::IBoardType::INPUT_TYPE_TBSCROSSFIRE:
        minVal = 172;
        maxVal = 1811;
        channelType = ManualControlSettings::CHANNELGROUPS_TBSCROSSFIRE;
        break;
    case Core::IBoardType::INPUT_TYPE_SRXL:
        // unfortunately Weatronic variant has different scaling, this is the range for standard
        // impl. per protocol spec.
        minVal = 585;
        maxVal = 3510;
        channelType = ManualControlSettings::CHANNELGROUPS_SRXL;
        break;
    case Core::IBoardType::INPUT_TYPE_UNKNOWN:
    case Core::IBoardType::INPUT_TYPE_DISABLED:
        channelType = ManualControlSettings::CHANNELGROUPS_NONE;
        break;
    case Core::IBoardType::INPUT_TYPE_ANY:
        break;
    }

    qint16 neutralVal = minVal + (maxVal - minVal) / 2;
    qint16 throttleNeutralVal = static_cast<qint16>(minVal + (maxVal - minVal) * 0.04);

    for (unsigned int i = 0; i < ManualControlSettings::CHANNELGROUPS_NUMELEM; i++) {
        cData.ChannelGroups[i] = channelType;
        cData.ChannelNumber[i] = 0;
        cData.ChannelMin[i] = minVal;
        cData.ChannelMax[i] = maxVal;
        cData.ChannelNeutral[i] = neutralVal;
    }

    cData.ChannelNumber[ManualControlSettings::CHANNELGROUPS_THROTTLE] = 1;
    cData.ChannelNeutral[ManualControlSettings::CHANNELNEUTRAL_THROTTLE] = throttleNeutralVal;
    cData.ChannelNumber[ManualControlSettings::CHANNELGROUPS_ROLL] = 2;
    cData.ChannelNumber[ManualControlSettings::CHANNELGROUPS_YAW] = 3;
    cData.ChannelNumber[ManualControlSettings::CHANNELGROUPS_PITCH] = 4;
    cData.ChannelNumber[ManualControlSettings::CHANNELGROUPS_FLIGHTMODE] = 5;

    mcSettings->setData(cData);
    addModifiedObject(mcSettings, tr("Writing manual control defaults"));
}

bool VehicleConfigurationHelper::saveChangesToController(bool save)
{
    qDebug() << "Saving modified objects to controller. " << m_modifiedObjects.count()
             << " objects in found.";
    const int OUTER_TIMEOUT = 3000 * 20; // 10 seconds timeout for saving all objects
    const int INNER_TIMEOUT = 2000; // 1 second timeout on every save attempt

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Q_ASSERT(pm);
    UAVObjectUtilManager *utilMngr = pm->getObject<UAVObjectUtilManager>();
    Q_ASSERT(utilMngr);

    QTimer outerTimeoutTimer;
    outerTimeoutTimer.setSingleShot(true);

    QTimer innerTimeoutTimer;
    innerTimeoutTimer.setSingleShot(true);

    connect(utilMngr, &UAVObjectUtilManager::saveCompleted, this,
            QOverload<int, bool>::of(&VehicleConfigurationHelper::uavoTransactionCompleted));
    connect(&innerTimeoutTimer, &QTimer::timeout, &m_eventLoop, &QEventLoop::quit);
    connect(&outerTimeoutTimer, &QTimer::timeout, this,
            &VehicleConfigurationHelper::saveChangesTimeout);

    outerTimeoutTimer.start(OUTER_TIMEOUT);
    for (int i = 0; i < m_modifiedObjects.count(); i++) {
        QPair<UAVDataObject *, QString> *objPair = m_modifiedObjects.at(i);
        m_transactionOK = false;
        UAVDataObject *obj = objPair->first;
        QString objDescription = objPair->second;
        if (UAVObject::GetGcsAccess(obj->getMetadata()) != UAVObject::ACCESS_READONLY
            && obj->isSettings()) {
            emit saveProgress(m_modifiedObjects.count() + 1, ++m_progress, objDescription);

            m_currentTransactionObjectID = obj->getObjID();

            connect(obj, QOverload<UAVObject *, bool>::of(&UAVObject::transactionCompleted), this,
                    QOverload<UAVObject *, bool>::of(
                        &VehicleConfigurationHelper::uavoTransactionCompleted));
            while (!m_transactionOK && !m_transactionTimeout) {
                // Allow the transaction to take some time
                innerTimeoutTimer.start(INNER_TIMEOUT);

                // Set object updated
                obj->updated();
                if (!m_transactionOK) {
                    m_eventLoop.exec();
                }
                innerTimeoutTimer.stop();
            }
            disconnect(obj, QOverload<UAVObject *, bool>::of(&UAVObject::transactionCompleted),
                       this, QOverload<UAVObject *, bool>::of(
                                 &VehicleConfigurationHelper::uavoTransactionCompleted));
            if (m_transactionOK) {
                qDebug() << "Object " << obj->getName() << " was successfully updated.";
                if (save) {
                    m_transactionOK = false;
                    m_currentTransactionObjectID = obj->getObjID();
                    // Try to save until success or timeout
                    while (!m_transactionOK && !m_transactionTimeout) {
                        // Allow the transaction to take some time
                        innerTimeoutTimer.start(INNER_TIMEOUT);

                        // Persist object in controller
                        utilMngr->saveObjectToFlash(obj);
                        if (!m_transactionOK) {
                            m_eventLoop.exec();
                        }
                        innerTimeoutTimer.stop();
                    }
                    m_currentTransactionObjectID = -1;
                }
            }

            if (!m_transactionOK) {
                qDebug() << "Transaction timed out when trying to save: " << obj->getName();
            } else {
                qDebug() << "Object " << obj->getName() << " was successfully saved.";
            }
        } else {
            qDebug()
                << "Trying to save a UAVDataObject that is read only or is not a settings object.";
        }
        if (m_transactionTimeout) {
            qDebug() << "Transaction timed out when trying to save " << m_modifiedObjects.count()
                     << " objects.";
            break;
        }
    }

    outerTimeoutTimer.stop();
    disconnect(&outerTimeoutTimer, &QTimer::timeout, this,
               &VehicleConfigurationHelper::saveChangesTimeout);
    disconnect(&innerTimeoutTimer, &QTimer::timeout, &m_eventLoop, &QEventLoop::quit);
    disconnect(utilMngr, &UAVObjectUtilManager::saveCompleted, this,
               QOverload<int, bool>::of(&VehicleConfigurationHelper::uavoTransactionCompleted));

    qDebug() << "Finished saving modified objects to controller. Success = " << m_transactionOK;

    return m_transactionOK;
}

void VehicleConfigurationHelper::uavoTransactionCompleted(int oid, bool success)
{
    if (oid == m_currentTransactionObjectID) {
        m_transactionOK = success;
        m_eventLoop.quit();
    }
}

void VehicleConfigurationHelper::uavoTransactionCompleted(UAVObject *object, bool success)
{
    if (object) {
        uavoTransactionCompleted(object->getObjID(), success);
    }
}

void VehicleConfigurationHelper::saveChangesTimeout()
{
    m_transactionOK = false;
    m_transactionTimeout = true;
    m_eventLoop.quit();
}

void VehicleConfigurationHelper::resetVehicleConfig()
{
    // Reset all vehicle data
    MixerSettings *mSettings = MixerSettings::GetInstance(m_uavoManager);

    // Reset throttle curves
    {
        UAVObjectField *field = mSettings->getField("ThrottleCurve2");
        Q_ASSERT(field);
        for (auto i = 0; i < field->getNumElements(); i++)
            field->setValue(i * (0.9f / (field->getNumElements() - 1)), i);
    }

    // Reset Mixer types and values
    QString mixerTypePattern = "Mixer%1Type";
    QString mixerVectorPattern = "Mixer%1Vector";
    for (int i = 1; i <= 10; i++) {
        UAVObjectField *field = mSettings->getField(mixerTypePattern.arg(i));
        Q_ASSERT(field);
        field->setValue(field->getOptions().at(0));

        field = mSettings->getField(mixerVectorPattern.arg(i));
        Q_ASSERT(field);
        for (auto i = 0; i < field->getNumElements(); i++)
            field->setValue(0, i);
    }

    // Apply updates
    // mSettings->setData(mSettings->getData());
    addModifiedObject(mSettings, tr("Preparing mixer settings"));
}

void VehicleConfigurationHelper::resetGUIData()
{
    SystemSettings *sSettings = SystemSettings::GetInstance(m_uavoManager);

    Q_ASSERT(sSettings);
    SystemSettings::DataFields data = sSettings->getData();
    data.AirframeType = SystemSettings::AIRFRAMETYPE_CUSTOM;
    for (quint32 i = 0; i < SystemSettings::AIRFRAMECATEGORYSPECIFICCONFIGURATION_NUMELEM; i++) {
        data.AirframeCategorySpecificConfiguration[i] = 0;
    }
    sSettings->setData(data);
    addModifiedObject(sSettings, tr("Preparing vehicle settings"));
}

void VehicleConfigurationHelper::setupTriCopter()
{
    // Typical vehicle setup
    // 1. Setup mixer data
    // 2. Setup GUI data
    // 3. Apply changes

    mixerChannelSettings channels[10];
    GUIConfigDataUnion guiSettings = getGUIConfigData();

    channels[0].type = MIXER_TYPE_MOTOR;
    channels[0].throttle1 = 100;
    channels[0].throttle2 = 0;
    channels[0].roll = 100;
    channels[0].pitch = 50;
    channels[0].yaw = 0;

    channels[1].type = MIXER_TYPE_MOTOR;
    channels[1].throttle1 = 100;
    channels[1].throttle2 = 0;
    channels[1].roll = -100;
    channels[1].pitch = 50;
    channels[1].yaw = 0;

    channels[2].type = MIXER_TYPE_MOTOR;
    channels[2].throttle1 = 100;
    channels[2].throttle2 = 0;
    channels[2].roll = 0;
    channels[2].pitch = -100;
    channels[2].yaw = 0;

    channels[3].type = MIXER_TYPE_SERVO;
    channels[3].throttle1 = 0;
    channels[3].throttle2 = 0;
    channels[3].roll = 0;
    channels[3].pitch = 0;
    channels[3].yaw = 100;

    guiSettings.multi.VTOLMotorNW = 1;
    guiSettings.multi.VTOLMotorNE = 2;
    guiSettings.multi.VTOLMotorS = 3;
    guiSettings.multi.TRIYaw = 4;

    applyMixerConfiguration(channels);
    applyMultiGUISettings(SystemSettings::AIRFRAMETYPE_TRI, guiSettings);
}

GUIConfigDataUnion VehicleConfigurationHelper::getGUIConfigData()
{
    GUIConfigDataUnion configData;

    for (int i = 0; i < (int)(SystemSettings::AIRFRAMECATEGORYSPECIFICCONFIGURATION_NUMELEM); i++) {
        configData.UAVObject[i] = 0; // systemSettingsData.GUIConfigData[i];
    }

    return configData;
}

void VehicleConfigurationHelper::setupQuadCopter()
{
    mixerChannelSettings channels[10];
    GUIConfigDataUnion guiSettings = getGUIConfigData();
    SystemSettings::AirframeTypeOptions frame = SystemSettings::AIRFRAMETYPE_QUADP;

    switch (m_configSource->getVehicleSubType()) {
    case VehicleConfigurationSource::MULTI_ROTOR_QUAD_PLUS: {
        frame = SystemSettings::AIRFRAMETYPE_QUADP;
        channels[0].type = MIXER_TYPE_MOTOR;
        channels[0].throttle1 = 100;
        channels[0].throttle2 = 0;
        channels[0].roll = 0;
        channels[0].pitch = 100;
        channels[0].yaw = -50;

        channels[1].type = MIXER_TYPE_MOTOR;
        channels[1].throttle1 = 100;
        channels[1].throttle2 = 0;
        channels[1].roll = -100;
        channels[1].pitch = 0;
        channels[1].yaw = 50;

        channels[2].type = MIXER_TYPE_MOTOR;
        channels[2].throttle1 = 100;
        channels[2].throttle2 = 0;
        channels[2].roll = 0;
        channels[2].pitch = -100;
        channels[2].yaw = -50;

        channels[3].type = MIXER_TYPE_MOTOR;
        channels[3].throttle1 = 100;
        channels[3].throttle2 = 0;
        channels[3].roll = 100;
        channels[3].pitch = 0;
        channels[3].yaw = 50;

        guiSettings.multi.VTOLMotorN = 1;
        guiSettings.multi.VTOLMotorE = 2;
        guiSettings.multi.VTOLMotorS = 3;
        guiSettings.multi.VTOLMotorW = 4;

        break;
    }
    case VehicleConfigurationSource::MULTI_ROTOR_QUAD_X: {
        frame = SystemSettings::AIRFRAMETYPE_QUADX;
        channels[0].type = MIXER_TYPE_MOTOR;
        channels[0].throttle1 = 100;
        channels[0].throttle2 = 0;
        channels[0].roll = 50;
        channels[0].pitch = 50;
        channels[0].yaw = -50;

        channels[1].type = MIXER_TYPE_MOTOR;
        channels[1].throttle1 = 100;
        channels[1].throttle2 = 0;
        channels[1].roll = -50;
        channels[1].pitch = 50;
        channels[1].yaw = 50;

        channels[2].type = MIXER_TYPE_MOTOR;
        channels[2].throttle1 = 100;
        channels[2].throttle2 = 0;
        channels[2].roll = -50;
        channels[2].pitch = -50;
        channels[2].yaw = -50;

        channels[3].type = MIXER_TYPE_MOTOR;
        channels[3].throttle1 = 100;
        channels[3].throttle2 = 0;
        channels[3].roll = 50;
        channels[3].pitch = -50;
        channels[3].yaw = 50;

        guiSettings.multi.VTOLMotorNW = 1;
        guiSettings.multi.VTOLMotorNE = 2;
        guiSettings.multi.VTOLMotorSE = 3;
        guiSettings.multi.VTOLMotorSW = 4;

        break;
    }
    default:
        break;
    }
    applyMixerConfiguration(channels);
    applyMultiGUISettings(frame, guiSettings);
}

void VehicleConfigurationHelper::setupHexaCopter()
{
    mixerChannelSettings channels[10];
    GUIConfigDataUnion guiSettings = getGUIConfigData();
    SystemSettings::AirframeTypeOptions frame = SystemSettings::AIRFRAMETYPE_HEXA;

    switch (m_configSource->getVehicleSubType()) {
    case VehicleConfigurationSource::MULTI_ROTOR_HEXA: {
        frame = SystemSettings::AIRFRAMETYPE_HEXA;

        channels[0].type = MIXER_TYPE_MOTOR;
        channels[0].throttle1 = 100;
        channels[0].throttle2 = 0;
        channels[0].roll = 0;
        channels[0].pitch = 33;
        channels[0].yaw = -33;

        channels[1].type = MIXER_TYPE_MOTOR;
        channels[1].throttle1 = 100;
        channels[1].throttle2 = 0;
        channels[1].roll = -50;
        channels[1].pitch = 33;
        channels[1].yaw = 33;

        channels[2].type = MIXER_TYPE_MOTOR;
        channels[2].throttle1 = 100;
        channels[2].throttle2 = 0;
        channels[2].roll = -50;
        channels[2].pitch = -33;
        channels[2].yaw = -33;

        channels[3].type = MIXER_TYPE_MOTOR;
        channels[3].throttle1 = 100;
        channels[3].throttle2 = 0;
        channels[3].roll = 0;
        channels[3].pitch = -33;
        channels[3].yaw = 33;

        channels[4].type = MIXER_TYPE_MOTOR;
        channels[4].throttle1 = 100;
        channels[4].throttle2 = 0;
        channels[4].roll = 50;
        channels[4].pitch = -33;
        channels[4].yaw = -33;

        channels[5].type = MIXER_TYPE_MOTOR;
        channels[5].throttle1 = 100;
        channels[5].throttle2 = 0;
        channels[5].roll = 50;
        channels[5].pitch = 33;
        channels[5].yaw = 33;

        guiSettings.multi.VTOLMotorN = 1;
        guiSettings.multi.VTOLMotorNE = 2;
        guiSettings.multi.VTOLMotorSE = 3;
        guiSettings.multi.VTOLMotorS = 4;
        guiSettings.multi.VTOLMotorSW = 5;
        guiSettings.multi.VTOLMotorNW = 6;

        break;
    }
    case VehicleConfigurationSource::MULTI_ROTOR_HEXA_COAX_Y: {
        frame = SystemSettings::AIRFRAMETYPE_HEXACOAX;

        channels[0].type = MIXER_TYPE_MOTOR;
        channels[0].throttle1 = 100;
        channels[0].throttle2 = 0;
        channels[0].roll = 100;
        channels[0].pitch = 25;
        channels[0].yaw = -66;

        channels[1].type = MIXER_TYPE_MOTOR;
        channels[1].throttle1 = 100;
        channels[1].throttle2 = 0;
        channels[1].roll = 100;
        channels[1].pitch = 25;
        channels[1].yaw = 66;

        channels[2].type = MIXER_TYPE_MOTOR;
        channels[2].throttle1 = 100;
        channels[2].throttle2 = 0;
        channels[2].roll = -100;
        channels[2].pitch = 25;
        channels[2].yaw = -66;

        channels[3].type = MIXER_TYPE_MOTOR;
        channels[3].throttle1 = 100;
        channels[3].throttle2 = 0;
        channels[3].roll = -100;
        channels[3].pitch = 25;
        channels[3].yaw = 66;

        channels[4].type = MIXER_TYPE_MOTOR;
        channels[4].throttle1 = 100;
        channels[4].throttle2 = 0;
        channels[4].roll = 0;
        channels[4].pitch = -50;
        channels[4].yaw = -66;

        channels[5].type = MIXER_TYPE_MOTOR;
        channels[5].throttle1 = 100;
        channels[5].throttle2 = 0;
        channels[5].roll = 0;
        channels[5].pitch = -50;
        channels[5].yaw = 66;

        guiSettings.multi.VTOLMotorNW = 1;
        guiSettings.multi.VTOLMotorW = 2;
        guiSettings.multi.VTOLMotorNE = 3;
        guiSettings.multi.VTOLMotorE = 4;
        guiSettings.multi.VTOLMotorS = 5;
        guiSettings.multi.VTOLMotorSE = 6;

        break;
    }
    case VehicleConfigurationSource::MULTI_ROTOR_HEXA_H: {
        frame = SystemSettings::AIRFRAMETYPE_HEXAX;

        channels[0].type = MIXER_TYPE_MOTOR;
        channels[0].throttle1 = 100;
        channels[0].throttle2 = 0;
        channels[0].roll = -33;
        channels[0].pitch = 50;
        channels[0].yaw = -33;

        channels[1].type = MIXER_TYPE_MOTOR;
        channels[1].throttle1 = 100;
        channels[1].throttle2 = 0;
        channels[1].roll = -33;
        channels[1].pitch = 0;
        channels[1].yaw = 33;

        channels[2].type = MIXER_TYPE_MOTOR;
        channels[2].throttle1 = 100;
        channels[2].throttle2 = 0;
        channels[2].roll = -33;
        channels[2].pitch = -50;
        channels[2].yaw = -33;

        channels[3].type = MIXER_TYPE_MOTOR;
        channels[3].throttle1 = 100;
        channels[3].throttle2 = 0;
        channels[3].roll = -33;
        channels[3].pitch = -50;
        channels[3].yaw = 33;

        channels[4].type = MIXER_TYPE_MOTOR;
        channels[4].throttle1 = 100;
        channels[4].throttle2 = 0;
        channels[4].roll = 33;
        channels[4].pitch = 0;
        channels[4].yaw = -33;

        channels[5].type = MIXER_TYPE_MOTOR;
        channels[5].throttle1 = 100;
        channels[5].throttle2 = 0;
        channels[5].roll = 33;
        channels[5].pitch = 50;
        channels[5].yaw = -33;

        guiSettings.multi.VTOLMotorNE = 1;
        guiSettings.multi.VTOLMotorE = 2;
        guiSettings.multi.VTOLMotorSE = 3;
        guiSettings.multi.VTOLMotorSW = 4;
        guiSettings.multi.VTOLMotorW = 5;
        guiSettings.multi.VTOLMotorNW = 6;

        break;
    }
    default:
        break;
    }
    applyMixerConfiguration(channels);
    applyMultiGUISettings(frame, guiSettings);
}

void VehicleConfigurationHelper::setupOctoCopter()
{
    mixerChannelSettings channels[10];
    GUIConfigDataUnion guiSettings = getGUIConfigData();
    SystemSettings::AirframeTypeOptions frame = SystemSettings::AIRFRAMETYPE_OCTO;

    switch (m_configSource->getVehicleSubType()) {
    case VehicleConfigurationSource::MULTI_ROTOR_OCTO: {
        frame = SystemSettings::AIRFRAMETYPE_OCTO;

        channels[0].type = MIXER_TYPE_MOTOR;
        channels[0].throttle1 = 100;
        channels[0].throttle2 = 0;
        channels[0].roll = 0;
        channels[0].pitch = 33;
        channels[0].yaw = -25;

        channels[1].type = MIXER_TYPE_MOTOR;
        channels[1].throttle1 = 100;
        channels[1].throttle2 = 0;
        channels[1].roll = -33;
        channels[1].pitch = 33;
        channels[1].yaw = 25;

        channels[2].type = MIXER_TYPE_MOTOR;
        channels[2].throttle1 = 100;
        channels[2].throttle2 = 0;
        channels[2].roll = -33;
        channels[2].pitch = 0;
        channels[2].yaw = -25;

        channels[3].type = MIXER_TYPE_MOTOR;
        channels[3].throttle1 = 100;
        channels[3].throttle2 = 0;
        channels[3].roll = -33;
        channels[3].pitch = -33;
        channels[3].yaw = 25;

        channels[4].type = MIXER_TYPE_MOTOR;
        channels[4].throttle1 = 100;
        channels[4].throttle2 = 0;
        channels[4].roll = 0;
        channels[4].pitch = -33;
        channels[4].yaw = -25;

        channels[5].type = MIXER_TYPE_MOTOR;
        channels[5].throttle1 = 100;
        channels[5].throttle2 = 0;
        channels[5].roll = 33;
        channels[5].pitch = -33;
        channels[5].yaw = 25;

        channels[6].type = MIXER_TYPE_MOTOR;
        channels[6].throttle1 = 100;
        channels[6].throttle2 = 0;
        channels[6].roll = 33;
        channels[6].pitch = 0;
        channels[6].yaw = -25;

        channels[7].type = MIXER_TYPE_MOTOR;
        channels[7].throttle1 = 100;
        channels[7].throttle2 = 0;
        channels[7].roll = 33;
        channels[7].pitch = 33;
        channels[7].yaw = 25;

        guiSettings.multi.VTOLMotorN = 1;
        guiSettings.multi.VTOLMotorNE = 2;
        guiSettings.multi.VTOLMotorE = 3;
        guiSettings.multi.VTOLMotorSE = 4;
        guiSettings.multi.VTOLMotorS = 5;
        guiSettings.multi.VTOLMotorSW = 6;
        guiSettings.multi.VTOLMotorW = 7;
        guiSettings.multi.VTOLMotorNW = 8;

        break;
    }
    case VehicleConfigurationSource::MULTI_ROTOR_OCTO_COAX_X: {
        frame = SystemSettings::AIRFRAMETYPE_OCTOCOAXX;

        channels[0].type = MIXER_TYPE_MOTOR;
        channels[0].throttle1 = 100;
        channels[0].throttle2 = 0;
        channels[0].roll = 50;
        channels[0].pitch = 50;
        channels[0].yaw = -50;

        channels[1].type = MIXER_TYPE_MOTOR;
        channels[1].throttle1 = 100;
        channels[1].throttle2 = 0;
        channels[1].roll = 50;
        channels[1].pitch = 50;
        channels[1].yaw = 50;

        channels[2].type = MIXER_TYPE_MOTOR;
        channels[2].throttle1 = 100;
        channels[2].throttle2 = 0;
        channels[2].roll = -50;
        channels[2].pitch = 50;
        channels[2].yaw = -50;

        channels[3].type = MIXER_TYPE_MOTOR;
        channels[3].throttle1 = 100;
        channels[3].throttle2 = 0;
        channels[3].roll = -50;
        channels[3].pitch = 50;
        channels[3].yaw = 50;

        channels[4].type = MIXER_TYPE_MOTOR;
        channels[4].throttle1 = 100;
        channels[4].throttle2 = 0;
        channels[4].roll = -50;
        channels[4].pitch = -50;
        channels[4].yaw = -50;

        channels[5].type = MIXER_TYPE_MOTOR;
        channels[5].throttle1 = 100;
        channels[5].throttle2 = 0;
        channels[5].roll = -50;
        channels[5].pitch = -50;
        channels[5].yaw = 50;

        channels[6].type = MIXER_TYPE_MOTOR;
        channels[6].throttle1 = 100;
        channels[6].throttle2 = 0;
        channels[6].roll = 50;
        channels[6].pitch = -50;
        channels[6].yaw = -50;

        channels[7].type = MIXER_TYPE_MOTOR;
        channels[7].throttle1 = 100;
        channels[7].throttle2 = 0;
        channels[7].roll = 50;
        channels[7].pitch = -50;
        channels[7].yaw = 50;

        guiSettings.multi.VTOLMotorNW = 1;
        guiSettings.multi.VTOLMotorN = 2;
        guiSettings.multi.VTOLMotorNE = 3;
        guiSettings.multi.VTOLMotorE = 4;
        guiSettings.multi.VTOLMotorSE = 5;
        guiSettings.multi.VTOLMotorS = 6;
        guiSettings.multi.VTOLMotorSW = 7;
        guiSettings.multi.VTOLMotorW = 8;

        break;
    }
    case VehicleConfigurationSource::MULTI_ROTOR_OCTO_COAX_PLUS: {
        frame = SystemSettings::AIRFRAMETYPE_OCTOCOAXP;

        channels[0].type = MIXER_TYPE_MOTOR;
        channels[0].throttle1 = 100;
        channels[0].throttle2 = 0;
        channels[0].roll = 0;
        channels[0].pitch = 100;
        channels[0].yaw = -50;

        channels[1].type = MIXER_TYPE_MOTOR;
        channels[1].throttle1 = 100;
        channels[1].throttle2 = 0;
        channels[1].roll = 0;
        channels[1].pitch = 100;
        channels[1].yaw = 50;

        channels[2].type = MIXER_TYPE_MOTOR;
        channels[2].throttle1 = 100;
        channels[2].throttle2 = 0;
        channels[2].roll = -100;
        channels[2].pitch = 0;
        channels[2].yaw = -50;

        channels[3].type = MIXER_TYPE_MOTOR;
        channels[3].throttle1 = 100;
        channels[3].throttle2 = 0;
        channels[3].roll = -100;
        channels[3].pitch = 0;
        channels[3].yaw = 50;

        channels[4].type = MIXER_TYPE_MOTOR;
        channels[4].throttle1 = 100;
        channels[4].throttle2 = 0;
        channels[4].roll = 0;
        channels[4].pitch = -100;
        channels[4].yaw = -50;

        channels[5].type = MIXER_TYPE_MOTOR;
        channels[5].throttle1 = 100;
        channels[5].throttle2 = 0;
        channels[5].roll = 0;
        channels[5].pitch = -100;
        channels[5].yaw = 50;

        channels[6].type = MIXER_TYPE_MOTOR;
        channels[6].throttle1 = 100;
        channels[6].throttle2 = 0;
        channels[6].roll = 100;
        channels[6].pitch = 0;
        channels[6].yaw = -50;

        channels[7].type = MIXER_TYPE_MOTOR;
        channels[7].throttle1 = 100;
        channels[7].throttle2 = 0;
        channels[7].roll = 100;
        channels[7].pitch = 0;
        channels[7].yaw = 50;

        guiSettings.multi.VTOLMotorN = 1;
        guiSettings.multi.VTOLMotorNE = 2;
        guiSettings.multi.VTOLMotorE = 3;
        guiSettings.multi.VTOLMotorSE = 4;
        guiSettings.multi.VTOLMotorS = 5;
        guiSettings.multi.VTOLMotorSW = 6;
        guiSettings.multi.VTOLMotorW = 7;
        guiSettings.multi.VTOLMotorNW = 8;

        break;
    }
    case VehicleConfigurationSource::MULTI_ROTOR_OCTO_V: {
        frame = SystemSettings::AIRFRAMETYPE_OCTOV;
        channels[0].type = MIXER_TYPE_MOTOR;
        channels[0].throttle1 = 100;
        channels[0].throttle2 = 0;
        channels[0].roll = -25;
        channels[0].pitch = 8;
        channels[0].yaw = -25;

        channels[1].type = MIXER_TYPE_MOTOR;
        channels[1].throttle1 = 100;
        channels[1].throttle2 = 0;
        channels[1].roll = -25;
        channels[1].pitch = 25;
        channels[1].yaw = 25;

        channels[2].type = MIXER_TYPE_MOTOR;
        channels[2].throttle1 = 100;
        channels[2].throttle2 = 0;
        channels[2].roll = -25;
        channels[2].pitch = -25;
        channels[2].yaw = -25;

        channels[3].type = MIXER_TYPE_MOTOR;
        channels[3].throttle1 = 100;
        channels[3].throttle2 = 0;
        channels[3].roll = -25;
        channels[3].pitch = -8;
        channels[3].yaw = 25;

        channels[4].type = MIXER_TYPE_MOTOR;
        channels[4].throttle1 = 100;
        channels[4].throttle2 = 0;
        channels[4].roll = 25;
        channels[4].pitch = -8;
        channels[4].yaw = -25;

        channels[5].type = MIXER_TYPE_MOTOR;
        channels[5].throttle1 = 100;
        channels[5].throttle2 = 0;
        channels[5].roll = 25;
        channels[5].pitch = -25;
        channels[5].yaw = 25;

        channels[6].type = MIXER_TYPE_MOTOR;
        channels[6].throttle1 = 100;
        channels[6].throttle2 = 0;
        channels[6].roll = 25;
        channels[6].pitch = 25;
        channels[6].yaw = -25;

        channels[7].type = MIXER_TYPE_MOTOR;
        channels[7].throttle1 = 100;
        channels[7].throttle2 = 0;
        channels[7].roll = 25;
        channels[7].pitch = 8;
        channels[7].yaw = 25;

        guiSettings.multi.VTOLMotorN = 1;
        guiSettings.multi.VTOLMotorNE = 2;
        guiSettings.multi.VTOLMotorE = 3;
        guiSettings.multi.VTOLMotorSE = 4;
        guiSettings.multi.VTOLMotorS = 5;
        guiSettings.multi.VTOLMotorSW = 6;
        guiSettings.multi.VTOLMotorW = 7;
        guiSettings.multi.VTOLMotorNW = 8;

        break;
    }
    default:
        break;
    }

    applyMixerConfiguration(channels);
    applyMultiGUISettings(frame, guiSettings);
}
