/**
 ******************************************************************************
 * @file       gcscontrol.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup GCSControl
 * @{
 * @brief GCSReceiver API
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

#include "gcscontrol.h"
#include <QDebug>
#include <QtPlugin>
#include "gcscontrolgadgetfactory.h"
#if defined(USE_SDL)
#include "sdlgamepad/sdlgamepad.h"
#endif

#define CHANNEL_MAX 9000
#define CHANNEL_NEUTRAL 5000
#define CHANNEL_THROTNEUTRAL 1800
#define CHANNEL_MIN 1000

bool GCSControl::firstInstance = true;

GCSControl::GCSControl()
    : hasControl(false)
{
    Q_ASSERT(firstInstance); // There should only be one instance of this class
    firstInstance = false;
    receiverActivity.setInterval(100);
    connect(&receiverActivity, &QTimer::timeout, this, &GCSControl::receiverActivitySlot);
}

void GCSControl::extensionsInitialized()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objMngr = pm->getObject<UAVObjectManager>();
    Q_ASSERT(objMngr);

    manControlSettingsUAVO = ManualControlSettings::GetInstance(objMngr);
    Q_ASSERT(manControlSettingsUAVO);

    m_gcsReceiver = GCSReceiver::GetInstance(objMngr);
    Q_ASSERT(m_gcsReceiver);
}

GCSControl::~GCSControl()
{
    // Delete this plugin from plugin manager
    removeObject(this);
}

bool GCSControl::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

#if defined(USE_SDL)
    sdlGamepad = new SDLGamepad();
    if (sdlGamepad->init()) {
        sdlGamepad->start();
        qRegisterMetaType<QListInt16>("QListInt16");
        qRegisterMetaType<ButtonNumber>("ButtonNumber");
    }
#endif

    mf = new GCSControlGadgetFactory(this);
    addAutoReleasedObject(mf);

    // Add this plugin to plugin manager
    addObject(this);
    return true;
}

void GCSControl::shutdown()
{
}

/**
 * @brief GCSControl::beginGCSControl Configure the FC to use the @ref GCSReceiver
 * control for manual control (caches and alters the @ref ManualControlSettings)
 */
bool GCSControl::beginGCSControl()
{
    if (hasControl)
        return false;
    dataBackup = manControlSettingsUAVO->getData();
    metaBackup = manControlSettingsUAVO->getMetadata();
    ManualControlSettings::Metadata meta = manControlSettingsUAVO->getDefaultMetadata();
    UAVObject::SetGcsAccess(meta, UAVObject::ACCESS_READWRITE);

    // No need to set flight mode, we leave that at none and directly change
    // the setting
    manControlSettingsUAVO->setChannelGroups(ManualControlSettings::CHANNELGROUPS_FLIGHTMODE,
                                             ManualControlSettings::CHANNELGROUPS_NONE);

    // Only throttle, roll, pitch, yaw, and arming need to be configured for this
    // mode to work
    const int NUM_CHANNELS = 5;
    const quint8 channels[NUM_CHANNELS] = { ManualControlSettings::CHANNELGROUPS_THROTTLE,
                                            ManualControlSettings::CHANNELGROUPS_ROLL,
                                            ManualControlSettings::CHANNELGROUPS_PITCH,
                                            ManualControlSettings::CHANNELGROUPS_YAW,
                                            ManualControlSettings::CHANNELGROUPS_ARMING };

    for (quint8 i = 0; i < NUM_CHANNELS; ++i) {
        quint8 x = channels[i];

        inverseMapping[x] = i;

        // Assign this channel to GCS control
        manControlSettingsUAVO->setChannelGroups(x, ManualControlSettings::CHANNELGROUPS_GCS);

        // Set the ranges to match what the widget produces
        manControlSettingsUAVO->setChannelNumber(x, i + 1);
        manControlSettingsUAVO->setChannelMax(x, CHANNEL_MAX);

        if (x == ManualControlSettings::CHANNELGROUPS_THROTTLE) {
            manControlSettingsUAVO->setChannelNeutral(x, CHANNEL_THROTNEUTRAL);
        } else {
            manControlSettingsUAVO->setChannelNeutral(x, CHANNEL_NEUTRAL);
        }

        manControlSettingsUAVO->setChannelMin(x, CHANNEL_MIN);
    }

    manControlSettingsUAVO->setDeadband(0);
    manControlSettingsUAVO->setFlightModeNumber(1);
    manControlSettingsUAVO->setFlightModePosition(
        0, ManualControlSettings::FLIGHTMODEPOSITION_STABILIZED1);
    manControlSettingsUAVO->updated();
    connect(manControlSettingsUAVO, &UAVObject::objectUpdated, this, &GCSControl::objectsUpdated);
    hasControl = true;

    for (quint8 x = 0; x < NUM_CHANNELS; ++x) {
        setChannel(channels[x], 0);
    }

    setChannel(ManualControlSettings::CHANNELGROUPS_ARMING, -1);
    receiverActivity.start();
    return true;
}

/**
 * @brief GCSControl::endGCSControl restores the cached @ref ManualControlSettings
 */
bool GCSControl::endGCSControl()
{
    if (!hasControl)
        return false;
    disconnect(manControlSettingsUAVO, &UAVObject::objectUpdated, this,
               &GCSControl::objectsUpdated);
    manControlSettingsUAVO->setData(dataBackup);
    manControlSettingsUAVO->setMetadata(metaBackup);
    manControlSettingsUAVO->updated();
    hasControl = false;
    receiverActivity.stop();
    return true;
}

bool GCSControl::setFlightMode(ManualControlSettings::FlightModePositionOptions flightMode)
{
    if (!hasControl)
        return false;
    manControlSettingsUAVO->setFlightModePosition(0, flightMode);
    manControlSettingsUAVO->updated();
    m_gcsReceiver->setChannel(ManualControlSettings::CHANNELGROUPS_FLIGHTMODE, CHANNEL_MIN);
    m_gcsReceiver->updated();
    return true;
}

bool GCSControl::setThrottle(float value)
{
    return setChannel(ManualControlSettings::CHANNELGROUPS_THROTTLE, value);
}

bool GCSControl::setRoll(float value)
{
    return setChannel(ManualControlSettings::CHANNELGROUPS_ROLL, value);
}

bool GCSControl::setPitch(float value)
{
    return setChannel(ManualControlSettings::CHANNELGROUPS_PITCH, value);
}

bool GCSControl::setYaw(float value)
{
    return setChannel(ManualControlSettings::CHANNELGROUPS_YAW, value);
}

bool GCSControl::setArming(float value)
{
    return setChannel(ManualControlSettings::CHANNELGROUPS_ARMING, value);
}

bool GCSControl::setChannel(quint8 channeltype, float value)
{
    if (channeltype >= ManualControlSettings::CHANNELGROUPS_NUMELEM)
        return false;

    uint8_t channel = inverseMapping[channeltype];

    if (value > 1 || value < -1 || channel >= GCSReceiver::CHANNEL_NUMELEM || !hasControl)
        return false;
    quint16 pwmValue;
    if (value >= 0)
        pwmValue = (value * (float)(CHANNEL_MAX - CHANNEL_NEUTRAL)) + (float)CHANNEL_NEUTRAL;
    else
        pwmValue = (value * (float)(CHANNEL_NEUTRAL - CHANNEL_MIN)) + (float)CHANNEL_NEUTRAL;
    m_gcsReceiver->setChannel(channel, pwmValue);
    m_gcsReceiver->updated();
    return true;
}

void GCSControl::objectsUpdated(UAVObject *obj)
{
    qDebug() << "GCSControl::objectsUpdated"
             << "Object" << obj->getName() << "changed outside this class";
}

void GCSControl::receiverActivitySlot()
{
    if (m_gcsReceiver)
        m_gcsReceiver->updated();
}
