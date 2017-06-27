/**
 ******************************************************************************
 * @file       aq32.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_AeroQuadPlugin AeroQuad boards support Plugin
 * @{
 * @brief Plugin to support AeroQuad boards
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

#include "aq32.h"

#include "uavobjects/uavobjectmanager.h"
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "hwaq32.h"

/**
 * @brief AQ32::AQ32
 *  This is the AQ32 board definition
 */
AQ32::AQ32(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));
    // Legacy USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_AEROQUAD_AQ32, DRONIN_PID_AEROQUAD_AQ32, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_AEROQUAD_AQ32, DRONIN_PID_AEROQUAD_AQ32, BCD_DEVICE_FIRMWARE));

    boardType = 0x94;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(5);
    channelBanks[0] = QVector<int>() << 1 << 2;
    channelBanks[1] = QVector<int>() << 3 << 4;
    channelBanks[2] = QVector<int>() << 5 << 6;
    channelBanks[3] = QVector<int>() << 7 << 8 << 9 << 10;
}

AQ32::~AQ32()
{
}

QString AQ32::shortName()
{
    return QString("AQ32");
}

QString AQ32::boardDescription()
{
    return QString("The AQ32 board");
}

//! Return which capabilities this board has
bool AQ32::queryCapabilities(BoardCapabilities capability)
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

QPixmap AQ32::getBoardPicture()
{
    return QPixmap(":/aq32/images/aq32.png");
}

//! Determine if this board supports configuring the receiver
bool AQ32::isInputConfigurationSupported(Core::IBoardType::InputType type)
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

QString AQ32::getHwUAVO()
{
    return "HwAQ32";
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @return true if successfully configured or false otherwise
 */
bool AQ32::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwAQ32 *hwAQ32 = HwAQ32::GetInstance(uavoManager);
    Q_ASSERT(hwAQ32);
    if (!hwAQ32)
        return false;

    HwAQ32::DataFields settings = hwAQ32->getData();

    switch (type) {
    case INPUT_TYPE_PPM:
        settings.RcvrPort = HwAQ32::RCVRPORT_PPM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.Uart3 = HwAQ32::UART3_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.Uart3 = HwAQ32::UART3_HOTTSUMH;
        break;
    case INPUT_TYPE_SBUS:
        settings.Uart3 = HwAQ32::UART3_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.Uart3 = HwAQ32::UART3_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_IBUS:
        settings.Uart3 = HwAQ32::UART3_IBUS;
        break;
    case INPUT_TYPE_DSM:
        settings.Uart4 = HwAQ32::UART4_DSM;
        break;
    case INPUT_TYPE_SRXL:
        settings.Uart3 = HwAQ32::UART3_SRXL;
        break;
    case INPUT_TYPE_TBSCROSSFIRE:
        settings.Uart3 = HwAQ32::UART3_TBSCROSSFIRE;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwAQ32->setData(settings);

    return true;
}

/**
 * @brief AQ32::getInputType fetch the currently selected input type
 * @return the selected input type
 */
Core::IBoardType::InputType AQ32::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwAQ32 *hwAQ32 = HwAQ32::GetInstance(uavoManager);
    Q_ASSERT(hwAQ32);
    if (!hwAQ32)
        return INPUT_TYPE_UNKNOWN;

    HwAQ32::DataFields settings = hwAQ32->getData();

    switch (settings.RcvrPort) {
    case HwAQ32::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    default:
        break;
    }

    switch (settings.Uart3) {
    case HwAQ32::UART3_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwAQ32::UART3_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwAQ32::UART3_SBUS:
        return INPUT_TYPE_SBUS;
    case HwAQ32::UART3_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwAQ32::UART3_IBUS:
        return INPUT_TYPE_IBUS;
    case HwAQ32::UART3_SRXL:
        return INPUT_TYPE_SRXL;
    case HwAQ32::UART3_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.Uart4) {
    case HwAQ32::UART4_DSM:
        return INPUT_TYPE_DSM;
    case HwAQ32::UART4_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwAQ32::UART4_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwAQ32::UART4_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwAQ32::UART4_IBUS:
        return INPUT_TYPE_IBUS;
    case HwAQ32::UART4_SRXL:
        return INPUT_TYPE_SRXL;
    case HwAQ32::UART4_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.Uart6) {
    case HwAQ32::UART6_DSM:
        return INPUT_TYPE_DSM;
    case HwAQ32::UART6_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwAQ32::UART6_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwAQ32::UART6_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwAQ32::UART6_IBUS:
        return INPUT_TYPE_IBUS;
    case HwAQ32::UART6_SRXL:
        return INPUT_TYPE_SRXL;
    case HwAQ32::UART6_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    return INPUT_TYPE_UNKNOWN;
}

int AQ32::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwAQ32 *hwAQ32 = HwAQ32::GetInstance(uavoManager);
    Q_ASSERT(hwAQ32);
    if (!hwAQ32)
        return 0;

    HwAQ32::DataFields settings = hwAQ32->getData();

    switch (settings.GyroRange) {
    case HwAQ32::GYRORANGE_250:
        return 250;
    case HwAQ32::GYRORANGE_500:
        return 500;
    case HwAQ32::GYRORANGE_1000:
        return 1000;
    case HwAQ32::GYRORANGE_2000:
        return 2000;
    default:
        break;
    }

    return 500;
}

QStringList AQ32::getAdcNames()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwAQ32 *hwAQ32 = HwAQ32::GetInstance(uavoManager);
    Q_ASSERT(hwAQ32);
    if (!hwAQ32)
        return QStringList();

    HwAQ32::DataFields settings = hwAQ32->getData();
    if (settings.ADCInputs == HwAQ32::ADCINPUTS_ENABLED) {
        return QStringList() << "AI2"
                             << "BM"
                             << "AI3"
                             << "AI4"
                             << "AI5";
    }

    return QStringList() << "Disabled"
                         << "Disabled"
                         << "Disabled"
                         << "Disabled"
                         << "Disabled";
}

bool AQ32::hasAnnunciator(AnnunciatorType annunc)
{
    switch (annunc) {
    case ANNUNCIATOR_HEARTBEAT:
    case ANNUNCIATOR_ALARM:
    case ANNUNCIATOR_BUZZER:
    case ANNUNCIATOR_RGB:
    case ANNUNCIATOR_DAC:
        return true;
    }
    return false;
}
