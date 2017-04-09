/**
 ******************************************************************************
 * @file       sparky.cpp
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

#include "sparky.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "hwsparky.h"

/**
 * @brief Sparky::Sparky
 *  This is the Sparky board definition
 */
Sparky::Sparky(void)
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

    boardType = 0x88;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(6);
    channelBanks[0] = QVector<int>() << 1 << 2; // TIM15
    channelBanks[1] = QVector<int>() << 3; // TIM1
    channelBanks[2] = QVector<int>() << 4 << 7 << 9; // TIM3
    channelBanks[3] = QVector<int>() << 5; // TIM16
    channelBanks[4] = QVector<int>() << 6 << 10; // TIM2
    channelBanks[5] = QVector<int>() << 8; // TIM17
}

Sparky::~Sparky()
{
}

QString Sparky::shortName()
{
    return QString("Sparky");
}

QString Sparky::boardDescription()
{
    return QString("The Tau Labs project Sparky boards");
}

//! Return which capabilities this board has
bool Sparky::queryCapabilities(BoardCapabilities capability)
{
    switch (capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_MAGS:
    case BOARD_CAPABILITIES_BAROS:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    default:
        break;
    }

    return false;
}

QPixmap Sparky::getBoardPicture()
{
    return QPixmap(":/taulabs/images/sparky.png");
}

QString Sparky::getHwUAVO()
{
    return "HwSparky";
}

//! Determine if this board supports configuring the receiver
bool Sparky::isInputConfigurationSupported(Core::IBoardType::InputType type)
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
bool Sparky::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwSparky *hwSparky = HwSparky::GetInstance(uavoManager);
    Q_ASSERT(hwSparky);
    if (!hwSparky)
        return false;

    HwSparky::DataFields settings = hwSparky->getData();

    switch (type) {
    case INPUT_TYPE_PPM:
        settings.RcvrPort = HwSparky::RCVRPORT_PPM;
        break;
    case INPUT_TYPE_SBUS:
        settings.RcvrPort = HwSparky::RCVRPORT_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.RcvrPort = HwSparky::RCVRPORT_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_DSM:
        settings.RcvrPort = HwSparky::RCVRPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.RcvrPort = HwSparky::RCVRPORT_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.RcvrPort = HwSparky::RCVRPORT_HOTTSUMH;
        break;
    case INPUT_TYPE_IBUS:
        settings.RcvrPort = HwSparky::RCVRPORT_IBUS;
        break;
    case INPUT_TYPE_SRXL:
        settings.RcvrPort = HwSparky::RCVRPORT_SRXL;
        break;
    case INPUT_TYPE_TBSCROSSFIRE:
        settings.RcvrPort = HwSparky::RCVRPORT_TBSCROSSFIRE;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwSparky->setData(settings);

    return true;
}

/**
 * @brief Sparky::getInputType fetch the currently selected input type
 * @return the selected input type
 */
Core::IBoardType::InputType Sparky::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwSparky *hwSparky = HwSparky::GetInstance(uavoManager);
    Q_ASSERT(hwSparky);
    if (!hwSparky)
        return INPUT_TYPE_UNKNOWN;

    HwSparky::DataFields settings = hwSparky->getData();

    switch (settings.RcvrPort) {
    case HwSparky::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwSparky::RCVRPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwSparky::RCVRPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwSparky::RCVRPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwSparky::RCVRPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwSparky::RCVRPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwSparky::RCVRPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwSparky::RCVRPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwSparky::RCVRPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.FlexiPort) {
    case HwSparky::FLEXIPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwSparky::FLEXIPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwSparky::FLEXIPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwSparky::FLEXIPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwSparky::FLEXIPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwSparky::FLEXIPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.MainPort) {
    case HwSparky::MAINPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwSparky::MAINPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwSparky::MAINPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwSparky::MAINPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwSparky::MAINPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    return INPUT_TYPE_UNKNOWN;
}

int Sparky::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwSparky *hwSparky = HwSparky::GetInstance(uavoManager);
    Q_ASSERT(hwSparky);
    if (!hwSparky)
        return 0;

    HwSparky::DataFields settings = hwSparky->getData();

    switch (settings.GyroRange) {
    case HwSparky::GYRORANGE_250:
        return 250;
    case HwSparky::GYRORANGE_500:
        return 500;
    case HwSparky::GYRORANGE_1000:
        return 1000;
    case HwSparky::GYRORANGE_2000:
        return 2000;
    default:
        break;
    }

    return 500;
}

QStringList Sparky::getAdcNames()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwSparky *hwSparky = HwSparky::GetInstance(uavoManager);
    Q_ASSERT(hwSparky);
    if (!hwSparky)
        return QStringList();

    QStringList names;
    HwSparky::DataFields settings = hwSparky->getData();
    if (settings.OutPort == HwSparky::OUTPORT_PWM82ADC
        || settings.OutPort == HwSparky::OUTPORT_PWM72ADCPWM_IN)
        names << "PWM10"
              << "PWM9"
              << "Disabled";
    else if (settings.OutPort == HwSparky::OUTPORT_PWM73ADC)
        names << "PWM10"
              << "PWM9"
              << "PWM8";
    else
        names << "Disabled"
              << "Disabled"
              << "Disabled";

    return names;
}
