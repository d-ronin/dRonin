/**
 ******************************************************************************
 * @file       matek405.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_MatekPlugin Matek boards support Plugin
 * @{
 * @brief Plugin to support Matek boards
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

#include "matek405.h"

#include "uavobjects/uavobjectmanager.h"
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "hwmatek405.h"

/**
 * @brief MATEK405::MATEK405
 *  This is the MATEK405 board definition
 */
MATEK405::MATEK405(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));
    
    boardType = 0x95;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(5);
    channelBanks[0] = QVector<int>() << 1;
    channelBanks[1] = QVector<int>() << 2<< 3 << 4;
	channelBanks[2] = QVector<int>() << 5;
	channelBanks[3] = QVector<int>() << 6;
	channelBanks[4] = QVector<int>() << 7;
}

MATEK405::~MATEK405()
{
}

QString MATEK405::shortName()
{
    return QString("MATEK405");
}

QString MATEK405::boardDescription()
{
    return QString("The MATEK405 board");
}

//! Return which capabilities this board has
bool MATEK405::queryCapabilities(BoardCapabilities capability)
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

QPixmap MATEK405::getBoardPicture()
{
    return QPixmap(":/matek/images/matek405.png");
}

//! Determine if this board supports configuring the receiver
bool MATEK405::isInputConfigurationSupported(Core::IBoardType::InputType type)
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

QString MATEK405::getHwUAVO()
{
    return "HwMatek405";
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @return true if successfully configured or false otherwise
 */
bool MATEK405::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwMatek405 *hwMatek405 = HwMatek405::GetInstance(uavoManager);
    Q_ASSERT(hwMatek405);
    if (!hwMatek405)
        return false;

    HwMatek405::DataFields settings = hwMatek405->getData();

    switch (type) {
    case INPUT_TYPE_PPM:
        settings.PPM_Receiver = HwMatek405::PPM_RECEIVER_ENABLED;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.Uart2 = HwMatek405::UART2_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.Uart2 = HwMatek405::UART2_HOTTSUMH;
        break;
    case INPUT_TYPE_SBUS:
        settings.Uart2 = HwMatek405::UART2_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.Uart2 = HwMatek405::UART2_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_IBUS:
        settings.Uart2 = HwMatek405::UART2_IBUS;
        break;
    case INPUT_TYPE_DSM:
        settings.Uart2 = HwMatek405::UART2_DSM;
        break;
    case INPUT_TYPE_SRXL:
        settings.Uart2 = HwMatek405::UART2_SRXL;
        break;
    case INPUT_TYPE_TBSCROSSFIRE:
        settings.Uart2 = HwMatek405::UART2_TBSCROSSFIRE;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwMatek405->setData(settings);

    return true;
}

/**
 * @brief MATEK405::getInputType fetch the currently selected input type
 * @return the selected input type
 */
Core::IBoardType::InputType MATEK405::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwMatek405 *hwMatek405 = HwMatek405::GetInstance(uavoManager);
    Q_ASSERT(hwMatek405);
    if (!hwMatek405)
        return INPUT_TYPE_UNKNOWN;

    HwMatek405::DataFields settings = hwMatek405->getData();

    switch (settings.PPM_Receiver) {
    case HwMatek405::PPM_RECEIVER_ENABLED:
        return INPUT_TYPE_PPM;
    default:
        break;
    }

    switch (settings.Uart2) {
    case HwMatek405::UART2_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwMatek405::UART2_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwMatek405::UART2_SBUS:
        return INPUT_TYPE_SBUS;
    case HwMatek405::UART2_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwMatek405::UART2_IBUS:
        return INPUT_TYPE_IBUS;
    case HwMatek405::UART2_SRXL:
        return INPUT_TYPE_SRXL;
    case HwMatek405::UART2_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    return INPUT_TYPE_UNKNOWN;
}

int MATEK405::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwMatek405 *hwMatek405 = HwMatek405::GetInstance(uavoManager);
    Q_ASSERT(hwMatek405);
    if (!hwMatek405)
        return 0;

    HwMatek405::DataFields settings = hwMatek405->getData();

    switch (settings.GyroRange) {
    case HwMatek405::GYRORANGE_250:
        return 250;
    case HwMatek405::GYRORANGE_500:
        return 500;
    case HwMatek405::GYRORANGE_1000:
        return 1000;
    case HwMatek405::GYRORANGE_2000:
        return 2000;
    default:
        break;
    }

    return 500;
}

QStringList MATEK405::getAdcNames()
{
    return QStringList() << "Current" << "Voltage" << "RSSI";
}

bool MATEK405::hasAnnunciator(AnnunciatorType annunc)
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
