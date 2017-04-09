/**
 ******************************************************************************
 * @file       sparky2.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_TauLabsPlugin Tau Labs boards support Plugin
 * @{
 * @brief Plugin to support boards by the Tau Labs project
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

#include "sparky2.h"

#include <uavobjectmanager.h>
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "rfm22bstatus.h"

/**
 * @brief Sparky2::Sparky2
 *  This is the Sparky board definition
 */
Sparky2::Sparky2(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));
    // Legacy USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_TAULABS_SPARKY, DRONIN_PID_TAULABS_SPARKY, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_TAULABS_SPARKY, DRONIN_PID_TAULABS_SPARKY, BCD_DEVICE_FIRMWARE));

    boardType = 0x92;

    // Define the bank of channels that are connected to a given timer
    //   Ch1 TIM3
    //   Ch2 TIM3
    //   Ch3 TIM9
    //   Ch4 TIM9
    //   Ch5 TIM5
    //   Ch6 TIM5
    //  LED1 TIM12
    //  LED2 TIM12
    //  LED3 TIM8
    //  LED4 TIM8
    channelBanks.resize(6);
    channelBanks[0] = QVector<int>() << 1 << 2;
    channelBanks[1] = QVector<int>() << 3 << 4;
    channelBanks[2] = QVector<int>() << 5 << 6;
    channelBanks[3] = QVector<int>() << 7 << 8;
    channelBanks[4] = QVector<int>() << 9 << 10;
    channelBanks[5] = QVector<int>();

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    uavoUtilManager = pm->getObject<UAVObjectUtilManager>();
}

Sparky2::~Sparky2()
{
}

QString Sparky2::shortName()
{
    return QString("Sparky2");
}

QString Sparky2::boardDescription()
{
    return QString("The Tau Labs project Sparky2 boards");
}

//! Return which capabilities this board has
bool Sparky2::queryCapabilities(BoardCapabilities capability)
{
    switch (capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_MAGS:
    case BOARD_CAPABILITIES_BAROS:
    case BOARD_CAPABILITIES_RADIO:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    default:
        break;
    }

    return false;
}

QPixmap Sparky2::getBoardPicture()
{
    return QPixmap(":/taulabs/images/sparky2.png");
}

QString Sparky2::getHwUAVO()
{
    return "HwSparky2";
}

//! Get the settings object
HwSparky2 *Sparky2::getSettings()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();

    HwSparky2 *hwSparky2 = HwSparky2::GetInstance(uavoManager);
    Q_ASSERT(hwSparky2);

    return hwSparky2;
}

//! Determine if this board supports configuring the receiver
bool Sparky2::isInputConfigurationSupported(Core::IBoardType::InputType type)
{
    switch (type) {
    case INPUT_TYPE_PWM:
    case INPUT_TYPE_UNKNOWN:
        return false;
    default:
        break;
    }

    return true;
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @return true if successfully configured or false otherwise
 */
bool Sparky2::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwSparky2 *hwSparky2 = HwSparky2::GetInstance(uavoManager);
    Q_ASSERT(hwSparky2);
    if (!hwSparky2)
        return false;

    HwSparky2::DataFields settings = hwSparky2->getData();

    switch (type) {
    case INPUT_TYPE_PPM:
        settings.RcvrPort = HwSparky2::RCVRPORT_PPM;
        break;
    case INPUT_TYPE_SBUS:
        settings.RcvrPort = HwSparky2::RCVRPORT_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.RcvrPort = HwSparky2::RCVRPORT_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_DSM:
        settings.RcvrPort = HwSparky2::RCVRPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.RcvrPort = HwSparky2::RCVRPORT_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.RcvrPort = HwSparky2::RCVRPORT_HOTTSUMH;
        break;
    case INPUT_TYPE_IBUS:
        settings.RcvrPort = HwSparky2::RCVRPORT_IBUS;
        break;
    case INPUT_TYPE_SRXL:
        settings.RcvrPort = HwSparky2::RCVRPORT_SRXL;
        break;
    case INPUT_TYPE_TBSCROSSFIRE:
        settings.RcvrPort = HwSparky2::RCVRPORT_TBSCROSSFIRE;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwSparky2->setData(settings);

    return true;
}

/**
 * @brief Sparky2::getInputType fetch the currently selected input type
 * @return the selected input type
 */
Core::IBoardType::InputType Sparky2::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwSparky2 *hwSparky2 = HwSparky2::GetInstance(uavoManager);
    Q_ASSERT(hwSparky2);
    if (!hwSparky2)
        return INPUT_TYPE_UNKNOWN;

    HwSparky2::DataFields settings = hwSparky2->getData();

    switch (settings.RcvrPort) {
    case HwSparky2::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwSparky2::RCVRPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwSparky2::RCVRPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwSparky2::RCVRPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwSparky2::RCVRPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwSparky2::RCVRPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwSparky2::RCVRPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwSparky2::RCVRPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwSparky2::RCVRPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.MainPort) {
    case HwSparky2::MAINPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwSparky2::MAINPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwSparky2::MAINPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwSparky2::MAINPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwSparky2::MAINPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwSparky2::MAINPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwSparky2::MAINPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.FlexiPort) {
    case HwSparky2::FLEXIPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwSparky2::FLEXIPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwSparky2::FLEXIPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwSparky2::FLEXIPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwSparky2::FLEXIPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwSparky2::FLEXIPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwSparky2::FLEXIPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    return INPUT_TYPE_UNKNOWN;
}

int Sparky2::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwSparky2 *hwSparky2 = HwSparky2::GetInstance(uavoManager);
    Q_ASSERT(hwSparky2);
    if (!hwSparky2)
        return 0;

    HwSparky2::DataFields settings = hwSparky2->getData();

    switch (settings.GyroRange) {
    case HwSparky2::GYRORANGE_250:
        return 250;
    case HwSparky2::GYRORANGE_500:
        return 500;
    case HwSparky2::GYRORANGE_1000:
        return 1000;
    case HwSparky2::GYRORANGE_2000:
        return 2000;
    default:
        break;
    }

    return 500;
}

/**
 * Get the RFM22b device ID this modem
 * @return RFM22B device ID or 0 if not supported
 */
quint32 Sparky2::getRfmID()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();

    // Flight controllers are instance 1
    RFM22BStatus *rfm22bStatus = RFM22BStatus::GetInstance(uavoManager, 1);
    Q_ASSERT(rfm22bStatus);
    RFM22BStatus::DataFields rfm22b = rfm22bStatus->getData();

    return rfm22b.DeviceID;
}

/**
 * Set the coordinator ID. If set to zero this device will
 * be a coordinator.
 * @return true if successful or false if not
 */
bool Sparky2::bindRadio(quint32 id, quint32 baud_rate, float rf_power,
                        Core::IBoardType::LinkMode linkMode, quint8 min, quint8 max)
{
    HwSparky2::DataFields settings = getSettings()->getData();

    settings.CoordID = id;

    switch (baud_rate) {
    case 9600:
        settings.MaxRfSpeed = HwSparky2::MAXRFSPEED_9600;
        break;
    case 19200:
        settings.MaxRfSpeed = HwSparky2::MAXRFSPEED_19200;
        break;
    case 32000:
        settings.MaxRfSpeed = HwSparky2::MAXRFSPEED_32000;
        break;
    case 64000:
        settings.MaxRfSpeed = HwSparky2::MAXRFSPEED_64000;
        break;
    case 100000:
        settings.MaxRfSpeed = HwSparky2::MAXRFSPEED_100000;
        break;
    case 192000:
        settings.MaxRfSpeed = HwSparky2::MAXRFSPEED_192000;
        break;
    }

    // Round to an integer to use a switch statement
    quint32 rf_power_100 = (rf_power * 100) + 0.5;
    switch (rf_power_100) {
    case 0:
        settings.MaxRfPower = HwSparky2::MAXRFPOWER_0;
        break;
    case 125:
        settings.MaxRfPower = HwSparky2::MAXRFPOWER_125;
        break;
    case 160:
        settings.MaxRfPower = HwSparky2::MAXRFPOWER_16;
        break;
    case 316:
        settings.MaxRfPower = HwSparky2::MAXRFPOWER_316;
        break;
    case 630:
        settings.MaxRfPower = HwSparky2::MAXRFPOWER_63;
        break;
    case 1260:
        settings.MaxRfPower = HwSparky2::MAXRFPOWER_126;
        break;
    case 2500:
        settings.MaxRfPower = HwSparky2::MAXRFPOWER_25;
        break;
    case 5000:
        settings.MaxRfPower = HwSparky2::MAXRFPOWER_50;
        break;
    case 10000:
        settings.MaxRfPower = HwSparky2::MAXRFPOWER_100;
        break;
    }

    switch (linkMode) {
    case Core::IBoardType::LINK_TELEM:
        settings.Radio = HwSparky2::RADIO_TELEM;
        break;
    case Core::IBoardType::LINK_TELEM_PPM:
        settings.Radio = HwSparky2::RADIO_TELEMPPM;
        break;
    case Core::IBoardType::LINK_PPM:
        settings.Radio = HwSparky2::RADIO_PPM;
        break;
    }

    settings.MinChannel = min;
    settings.MaxChannel = max;

    getSettings()->setData(settings);
    uavoUtilManager->saveObjectToFlash(getSettings());

    return true;
}

QStringList Sparky2::getAdcNames()
{
    return QStringList() << "Analog CUR"
                         << "Analog VOLT"
                         << "DAC";
}
