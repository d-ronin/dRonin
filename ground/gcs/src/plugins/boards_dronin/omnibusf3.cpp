/**
 ******************************************************************************
 *
 * @file       omnibusf3.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2017
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_dRonin dRonin board support plugin
 * @{
 * @brief Support the Piko BLX flight controller
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

#include "omnibusf3.h"

#include <QIcon>

#include "uavobjects/uavobjectmanager.h"
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "hwomnibusf3.h"

/**
 * @brief OmnibusF3::OmnibusF3
 *  This is the OmnibusF3 board definition
 */
OmnibusF3::OmnibusF3(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));

    boardType = 0xCE;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(3);
    channelBanks[0] = QVector<int>() << 1 << 2; // 5 << 6
    channelBanks[1] = QVector<int>() << 3;
    channelBanks[2] = QVector<int>() << 4;
    //channelBanks[3] = QVector<int>() << 7 << 8;
}

OmnibusF3::~OmnibusF3()
{
}

QString OmnibusF3::shortName()
{
    return QString("OmnibusF3");
}

QString OmnibusF3::boardDescription()
{
    return QString("OmnibusF3 flight controller");
}

//! Return which capabilities this board has
bool OmnibusF3::queryCapabilities(BoardCapabilities capability)
{
    switch (capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    case BOARD_CAPABILITIES_MAGS:
    case BOARD_CAPABILITIES_BAROS:
    default:
        break;
    }

    return false;
}

QPixmap OmnibusF3::getBoardPicture()
{
    return QPixmap(":/dronin/images/omnibusf3nano.png");
}

//! Determine if this board supports configuring the receiver
bool OmnibusF3::isInputConfigurationSupported(Core::IBoardType::InputType type)
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

QString OmnibusF3::getHwUAVO()
{
    return "HwOmnibusF3";
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @return true if successfully configured or false otherwise
 */
bool OmnibusF3::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwOmnibusF3 *HwOmnibusF3 = HwOmnibusF3::GetInstance(uavoManager);
    Q_ASSERT(HwOmnibusF3);
    if (!HwOmnibusF3)
        return false;

    HwOmnibusF3::DataFields settings = HwOmnibusF3->getData();

    switch (type) {
    case INPUT_TYPE_PPM:
        settings.Uart3 = HwOmnibusF3::UART3_PPM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.Uart3 = HwOmnibusF3::UART3_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.Uart3 = HwOmnibusF3::UART3_HOTTSUMD;
        break;
    case INPUT_TYPE_SBUS:
        settings.Uart3 = HwOmnibusF3::UART3_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.Uart3 = HwOmnibusF3::UART3_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_IBUS:
        settings.Uart3 = HwOmnibusF3::UART3_IBUS;
        break;
    case INPUT_TYPE_SRXL:
        settings.Uart3 = HwOmnibusF3::UART3_SRXL;
        break;
    case INPUT_TYPE_DSM:
        settings.Uart3 = HwOmnibusF3::UART3_DSM;
        break;
    case INPUT_TYPE_TBSCROSSFIRE:
        settings.Uart3 = HwOmnibusF3::UART3_TBSCROSSFIRE;
        break;
    default:
        return false;
    }

    // Apply these changes
    HwOmnibusF3->setData(settings);

    return true;
}

/**
 * @brief OmnibusF3::getInputType fetch the currently selected input type
 * @return the selected input type
 */
Core::IBoardType::InputType OmnibusF3::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwOmnibusF3 *HwOmnibusF3 = HwOmnibusF3::GetInstance(uavoManager);
    Q_ASSERT(HwOmnibusF3);
    if (!HwOmnibusF3)
        return INPUT_TYPE_UNKNOWN;

    HwOmnibusF3::DataFields settings = HwOmnibusF3->getData();

    switch (settings.Uart3) {
    case HwOmnibusF3::UART3_PPM:
        return INPUT_TYPE_PPM;
    case HwOmnibusF3::UART3_DSM:
        return INPUT_TYPE_DSM;
    case HwOmnibusF3::UART3_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwOmnibusF3::UART3_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwOmnibusF3::UART3_SBUS:
        return INPUT_TYPE_SBUS;
    case HwOmnibusF3::UART3_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwOmnibusF3::UART3_IBUS:
        return INPUT_TYPE_IBUS;
    case HwOmnibusF3::UART3_SRXL:
        return INPUT_TYPE_SRXL;
    case HwOmnibusF3::UART3_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.Uart1) {
    case HwOmnibusF3::UART1_DSM:
        return INPUT_TYPE_DSM;
    case HwOmnibusF3::UART1_SBUS:
        return INPUT_TYPE_SBUS;
    case HwOmnibusF3::UART1_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwOmnibusF3::UART1_IBUS:
        return INPUT_TYPE_IBUS;
    case HwOmnibusF3::UART1_SRXL:
        return INPUT_TYPE_SRXL;
    case HwOmnibusF3::UART1_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwOmnibusF3::UART1_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwOmnibusF3::UART1_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.Uart2) {
    case HwOmnibusF3::UART2_DSM:
        return INPUT_TYPE_DSM;
    case HwOmnibusF3::UART2_SBUS:
        return INPUT_TYPE_SBUS;
    case HwOmnibusF3::UART2_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwOmnibusF3::UART2_IBUS:
        return INPUT_TYPE_IBUS;
    case HwOmnibusF3::UART2_SRXL:
        return INPUT_TYPE_SRXL;
    case HwOmnibusF3::UART2_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwOmnibusF3::UART2_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwOmnibusF3::UART2_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    return INPUT_TYPE_UNKNOWN;
}

int OmnibusF3::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwOmnibusF3 *hwOmnibusF3 = HwOmnibusF3::GetInstance(uavoManager);
    Q_ASSERT(hwOmnibusF3);
    if (!hwOmnibusF3)
        return 0;

    HwOmnibusF3::DataFields settings = hwOmnibusF3->getData();

    switch (settings.GyroRange) {
    case HwOmnibusF3::GYRORANGE_250:
        return 250;
    case HwOmnibusF3::GYRORANGE_500:
        return 500;
    case HwOmnibusF3::GYRORANGE_1000:
        return 1000;
    case HwOmnibusF3::GYRORANGE_2000:
        return 2000;
    default:
        break;
    }

    return 500;
}

QStringList OmnibusF3::getAdcNames()
{
    return QStringList() << "V"
                         << "I";
}

bool OmnibusF3::hasAnnunciator(AnnunciatorType annunc)
{
    switch (annunc) {
    case ANNUNCIATOR_HEARTBEAT:
    case ANNUNCIATOR_ALARM:
    case ANNUNCIATOR_BUZZER:
    case ANNUNCIATOR_RGB:
        return true;
    case ANNUNCIATOR_DAC:
        break;
    }
    return false;
}
