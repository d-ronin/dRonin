/**
 ******************************************************************************
 * @file       kakutef4v2.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_HolybroPlugin Holybro boards support Plugin
 * @{
 * @brief Plugin to support Holybro boards
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

#include "kakutef4v2.h"

#include "uavobjects/uavobjectmanager.h"
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "hwkakutef4v2.h"

/**
 * @brief KAKUTEF4V2::KAKUTEF4V2
 *  This is the KAKUTEF4V2 board definition
 */
KAKUTEF4V2::KAKUTEF4V2(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));
    
    boardType = 0x97;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(3);
    channelBanks[0] = QVector<int>() << 1 << 2;
    channelBanks[1] = QVector<int>() << 3;
	channelBanks[2] = QVector<int>() << 4;
}

KAKUTEF4V2::~KAKUTEF4V2()
{
}

QString KAKUTEF4V2::shortName()
{
    return QString("KAKUTEF4V2");
}

QString KAKUTEF4V2::boardDescription()
{
    return QString("The KAKUTEF4V2 board");
}

//! Return which capabilities this board has
bool KAKUTEF4V2::queryCapabilities(BoardCapabilities capability)
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

QPixmap KAKUTEF4V2::getBoardPicture()
{
    return QPixmap(":/matek/images/kakutef4v2.png");
}

//! Determine if this board supports configuring the receiver
bool KAKUTEF4V2::isInputConfigurationSupported(Core::IBoardType::InputType type)
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

QString KAKUTEF4V2::getHwUAVO()
{
    return "HwKakutef4v2";
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @return true if successfully configured or false otherwise
 */
bool KAKUTEF4V2::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwKakutef4v2 *hwKakutef4v2 = HwKakutef4v2::GetInstance(uavoManager);
    Q_ASSERT(hwKakutef4v2);
    if (!hwKakutef4v2)
        return false;

    HwKakutef4v2::DataFields settings = hwKakutef4v2->getData();

    switch (type) {
    case INPUT_TYPE_HOTTSUMD:
        settings.Uart3 = HwKakutef4v2::UART3_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.Uart3 = HwKakutef4v2::UART3_HOTTSUMH;
        break;
    case INPUT_TYPE_SBUS:
        settings.Uart3 = HwKakutef4v2::UART3_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.Uart3 = HwKakutef4v2::UART3_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_IBUS:
        settings.Uart3 = HwKakutef4v2::UART3_IBUS;
        break;
    case INPUT_TYPE_DSM:
        settings.Uart3 = HwKakutef4v2::UART3_DSM;
        break;
    case INPUT_TYPE_SRXL:
        settings.Uart3 = HwKakutef4v2::UART3_SRXL;
        break;
    case INPUT_TYPE_TBSCROSSFIRE:
        settings.Uart3 = HwKakutef4v2::UART3_TBSCROSSFIRE;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwKakutef4v2->setData(settings);

    return true;
}

/**
 * @brief KAKUTEF4V2::getInputType fetch the currently selected input type
 * @return the selected input type
 */
Core::IBoardType::InputType KAKUTEF4V2::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwKakutef4v2 *hwKakutef4v2 = HwKakutef4v2::GetInstance(uavoManager);
    Q_ASSERT(hwKakutef4v2);
    if (!hwKakutef4v2)
        return INPUT_TYPE_UNKNOWN;

    HwKakutef4v2::DataFields settings = hwKakutef4v2->getData();

    switch (settings.Uart3) {
    case HwKakutef4v2::UART3_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwKakutef4v2::UART3_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwKakutef4v2::UART3_SBUS:
        return INPUT_TYPE_SBUS;
    case HwKakutef4v2::UART3_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwKakutef4v2::UART3_IBUS:
        return INPUT_TYPE_IBUS;
    case HwKakutef4v2::UART3_SRXL:
        return INPUT_TYPE_SRXL;
    case HwKakutef4v2::UART3_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    return INPUT_TYPE_UNKNOWN;
}

int KAKUTEF4V2::queryMaxGyroRate()
{
    return 2000;
}

QStringList KAKUTEF4V2::getAdcNames()
{
    return QStringList() << "Current" << "Voltage" << "RSSI/Fdbk1" << "Fdbk2";
}

bool KAKUTEF4V2::hasAnnunciator(AnnunciatorType annunc)
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
