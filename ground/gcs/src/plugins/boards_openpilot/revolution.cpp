/**
 ******************************************************************************
 *
 * @file       revolution.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_OpenPilotPlugin OpenPilot boards support Plugin
 * @{
 * @brief Plugin to support boards by the OP project
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

#include "revolution.h"
#include "rfm22bstatus.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>

#include "hwrevolution.h"

/**
 * @brief Revolution::Revolution
 *  This is the Revolution board definition
 */
Revolution::Revolution(void)
{
    // Initialize our USB Structure definition here:
    USBInfo board;
    board.vendorID = 0x20A0;
    board.productID = 0x415b;

    setUSBInfo(board);

    boardType = 0x09;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(6);
    channelBanks[0] = QVector<int> () << 1 << 2;
    channelBanks[1] = QVector<int> () << 3;
    channelBanks[2] = QVector<int> () << 4;
    channelBanks[3] = QVector<int> () << 5 << 6;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    uavoUtilManager = pm->getObject<UAVObjectUtilManager>();
}

Revolution::~Revolution()
{

}

QString Revolution::shortName()
{
    return QString("Revolution");
}

QString Revolution::boardDescription()
{
    return QString("The OpenPilot project Revolution board");
}

//! Return which capabilities this board has
bool Revolution::queryCapabilities(BoardCapabilities capability)
{
    switch(capability) {
    case BOARD_CAPABILITIES_GYROS:
        return true;
    case BOARD_CAPABILITIES_ACCELS:
        return true;
    case BOARD_CAPABILITIES_MAGS:
        return true;
    case BOARD_CAPABILITIES_BAROS:
        return true;
    case BOARD_CAPABILITIES_RADIO:
        return true;
    case BOARD_CAPABILITIES_OSD:
        return false;
    }
    return false;
}

/**
 * @brief Revolution::getSupportedProtocols
 *  TODO: this is just a stub, we'll need to extend this a lot with multi protocol support
 * @return
 */
QStringList Revolution::getSupportedProtocols()
{

    return QStringList("uavtalk");
}

QPixmap Revolution::getBoardPicture()
{
    return QPixmap(":/openpilot/images/revolution.png");
}

QString Revolution::getHwUAVO()
{
    return "HwRevolution";
}

//! Get the settings object
HwRevolution * Revolution::getSettings()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();

    HwRevolution *hwRevolution = HwRevolution::GetInstance(uavoManager);
    Q_ASSERT(hwRevolution);

    return hwRevolution;
}
//! Determine if this board supports configuring the receiver
bool Revolution::isInputConfigurationSupported()
{
    return true;
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @return true if successfully configured or false otherwise
 */
bool Revolution::setInputType(enum InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwRevolution *hwRevolution = HwRevolution::GetInstance(uavoManager);
    Q_ASSERT(hwRevolution);
    if (!hwRevolution)
        return false;

    HwRevolution::DataFields settings = hwRevolution->getData();

    // Default to serial telemetry on the serial port
    settings.MainPort = HwRevolution::MAINPORT_TELEMETRY;

    switch(type) {
    case INPUT_TYPE_PWM:
        settings.RcvrPort = HwRevolution::RCVRPORT_PWM;
        break;
    case INPUT_TYPE_PPM:
        settings.RcvrPort = HwRevolution::RCVRPORT_PPM;
        break;
    case INPUT_TYPE_SBUS:
        settings.FlexiPort = HwRevolution::FLEXIPORT_TELEMETRY;
        settings.MainPort = HwRevolution::MAINPORT_SBUS;
        break;
    case INPUT_TYPE_DSM:
        settings.FlexiPort = HwRevolution::FLEXIPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.FlexiPort = HwRevolution::FLEXIPORT_HOTTSUMD;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwRevolution->setData(settings);

    return true;
}

/**
 * @brief Revolution::getInputType fetch the currently selected input type
 * @return the selected input type
 */
enum Core::IBoardType::InputType Revolution::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwRevolution *hwRevolution = HwRevolution::GetInstance(uavoManager);
    Q_ASSERT(hwRevolution);
    if (!hwRevolution)
        return INPUT_TYPE_UNKNOWN;

    HwRevolution::DataFields settings = hwRevolution->getData();

    switch(settings.FlexiPort) {
    case HwRevolution::FLEXIPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwRevolution::FLEXIPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    default:
        break;
    }

    switch(settings.MainPort) {
    case HwRevolution::MAINPORT_SBUS:
        return INPUT_TYPE_SBUS;
    default:
        break;
    }

    switch(settings.RcvrPort) {
    case HwRevolution::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwRevolution::RCVRPORT_PWM:
        return INPUT_TYPE_PWM;
    default:
        break;
    }

    return INPUT_TYPE_UNKNOWN;
}

int Revolution::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwRevolution *hwRevolution = HwRevolution::GetInstance(uavoManager);
    Q_ASSERT(hwRevolution);
    if (!hwRevolution)
        return 0;

    HwRevolution::DataFields settings = hwRevolution->getData();

    switch(settings.GyroRange) {
    case HwRevolution::GYRORANGE_250:
        return 250;
    case HwRevolution::GYRORANGE_500:
        return 500;
    case HwRevolution::GYRORANGE_1000:
        return 1000;
    case HwRevolution::GYRORANGE_2000:
        return 2000;
    default:
        return 500;
    }
}


/**
 * Get the RFM22b device ID this modem
 * @return RFM22B device ID or 0 if not supported
 */
quint32 Revolution::getRfmID()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();

    // Flight controllers are instance 1
    RFM22BStatus *rfm22bStatus = RFM22BStatus::GetInstance(uavoManager,1);
    Q_ASSERT(rfm22bStatus);
    RFM22BStatus::DataFields rfm22b = rfm22bStatus->getData();

    return rfm22b.DeviceID;
}

/**
 * Set the coordinator ID. If set to zero this device will
 * be a coordinator.
 * @return true if successful or false if not
 */
bool Revolution::bindRadio(quint32 id, quint32 baud_rate, float rf_power,
                         Core::IBoardType::LinkMode linkMode, quint8 min, quint8 max)
{
    HwRevolution::DataFields settings = getSettings()->getData();

    settings.CoordID = id;

    switch(baud_rate) {
    case 9600:
        settings.MaxRfSpeed = HwRevolution::MAXRFSPEED_9600;
        break;
    case 19200:
        settings.MaxRfSpeed = HwRevolution::MAXRFSPEED_19200;
        break;
    case 32000:
        settings.MaxRfSpeed = HwRevolution::MAXRFSPEED_32000;
        break;
    case 64000:
        settings.MaxRfSpeed = HwRevolution::MAXRFSPEED_64000;
        break;
    case 100000:
        settings.MaxRfSpeed = HwRevolution::MAXRFSPEED_100000;
        break;
    case 192000:
        settings.MaxRfSpeed = HwRevolution::MAXRFSPEED_192000;
        break;
    }

    // Round to an integer to use a switch statement
    quint32 rf_power_100 = rf_power * 100;
    switch(rf_power_100) {
    case 0:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_0;
        break;
    case 125:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_125;
        break;
    case 160:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_16;
        break;
    case 316:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_316;
        break;
    case 630:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_63;
        break;
    case 1260:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_126;
        break;
    case 2500:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_25;
        break;
    case 5000:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_50;
        break;
    case 10000:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_100;
        break;
    }

    switch(linkMode) {
    case Core::IBoardType::LINK_TELEM:
        settings.Radio = HwRevolution::RADIO_TELEM;
        break;
    case Core::IBoardType::LINK_TELEM_PPM:
        settings.Radio = HwRevolution::RADIO_TELEMPPM;
        break;
    case Core::IBoardType::LINK_PPM:
        settings.Radio = HwRevolution::RADIO_PPM;
        break;
    }

    settings.MinChannel = min;
    settings.MaxChannel = max;

    getSettings()->setData(settings);
    uavoUtilManager->saveObjectToFlash(getSettings());

    return true;
}

QStringList Revolution::getAdcNames()
{
    return QStringList() << "Pwr Sen Pin 3" << "Pwr Sen Pin 4";
}
