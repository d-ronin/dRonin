/**
 ******************************************************************************
 *
 * @file       pikoblx.cpp
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

#include "pikoblx.h"

#include <QIcon>

#include "uavobjects/uavobjectmanager.h"
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "hwpikoblx.h"

/**
 * @brief PikoBLX::PikoBLX
 *  This is the PikoBLX board definition
 */
PikoBLX::PikoBLX(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));

    boardType = 0xA2;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(6);
    channelBanks[0] = QVector<int>() << 1 << 2 << 3 << 4;
    channelBanks[1] = QVector<int>() << 5 << 6;
    channelBanks[2] = QVector<int>() << 7;
    channelBanks[3] = QVector<int>() << 8;
}

PikoBLX::~PikoBLX()
{
}

QString PikoBLX::shortName()
{
    return QString("pikoblx");
}

QString PikoBLX::boardDescription()
{
    return QString("pikoblx flight controller");
}

//! Return which capabilities this board has
bool PikoBLX::queryCapabilities(BoardCapabilities capability)
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

QPixmap PikoBLX::getBoardPicture()
{
    return QIcon(":/dronin/images/pikoblx.svg").pixmap(QSize(735, 718));
}

//! Determine if this board supports configuring the receiver
bool PikoBLX::isInputConfigurationSupported(Core::IBoardType::InputType type)
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

QString PikoBLX::getHwUAVO()
{
    return "HwPikoBLX";
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @return true if successfully configured or false otherwise
 */
bool PikoBLX::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwPikoBLX *HwPikoBLX = HwPikoBLX::GetInstance(uavoManager);
    Q_ASSERT(HwPikoBLX);
    if (!HwPikoBLX)
        return false;

    HwPikoBLX::DataFields settings = HwPikoBLX->getData();

    switch (type) {
    case INPUT_TYPE_PPM:
        settings.RxPort = HwPikoBLX::RXPORT_PPM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.RxPort = HwPikoBLX::RXPORT_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.RxPort = HwPikoBLX::RXPORT_HOTTSUMD;
        break;
    case INPUT_TYPE_SBUS:
        settings.RxPort = HwPikoBLX::RXPORT_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.RxPort = HwPikoBLX::RXPORT_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_IBUS:
        settings.RxPort = HwPikoBLX::RXPORT_IBUS;
        break;
    case INPUT_TYPE_SRXL:
        settings.RxPort = HwPikoBLX::RXPORT_SRXL;
        break;
    case INPUT_TYPE_DSM:
        settings.RxPort = HwPikoBLX::RXPORT_DSM;
        break;
    case INPUT_TYPE_TBSCROSSFIRE:
        settings.RxPort = HwPikoBLX::RXPORT_TBSCROSSFIRE;
        break;
    default:
        return false;
    }

    // Apply these changes
    HwPikoBLX->setData(settings);

    return true;
}

/**
 * @brief PikoBLX::getInputType fetch the currently selected input type
 * @return the selected input type
 */
Core::IBoardType::InputType PikoBLX::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwPikoBLX *HwPikoBLX = HwPikoBLX::GetInstance(uavoManager);
    Q_ASSERT(HwPikoBLX);
    if (!HwPikoBLX)
        return INPUT_TYPE_UNKNOWN;

    HwPikoBLX::DataFields settings = HwPikoBLX->getData();

    switch (settings.RxPort) {
    case HwPikoBLX::RXPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwPikoBLX::RXPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwPikoBLX::RXPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwPikoBLX::RXPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwPikoBLX::RXPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwPikoBLX::RXPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwPikoBLX::RXPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwPikoBLX::RXPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwPikoBLX::RXPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.Uart1) {
    case HwPikoBLX::UART1_DSM:
        return INPUT_TYPE_DSM;
    case HwPikoBLX::UART1_SBUS:
        return INPUT_TYPE_SBUS;
    case HwPikoBLX::UART1_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwPikoBLX::UART1_IBUS:
        return INPUT_TYPE_IBUS;
    case HwPikoBLX::UART1_SRXL:
        return INPUT_TYPE_SRXL;
    case HwPikoBLX::UART1_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwPikoBLX::UART1_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwPikoBLX::UART1_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.Uart2) {
    case HwPikoBLX::UART2_DSM:
        return INPUT_TYPE_DSM;
    case HwPikoBLX::UART2_SBUS:
        return INPUT_TYPE_SBUS;
    case HwPikoBLX::UART2_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwPikoBLX::UART2_IBUS:
        return INPUT_TYPE_IBUS;
    case HwPikoBLX::UART2_SRXL:
        return INPUT_TYPE_SRXL;
    case HwPikoBLX::UART2_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwPikoBLX::UART2_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwPikoBLX::UART2_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    return INPUT_TYPE_UNKNOWN;
}

int PikoBLX::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwPikoBLX *hwPikoBLX = HwPikoBLX::GetInstance(uavoManager);
    Q_ASSERT(hwPikoBLX);
    if (!hwPikoBLX)
        return 0;

    HwPikoBLX::DataFields settings = hwPikoBLX->getData();

    switch (settings.GyroRange) {
    case HwPikoBLX::GYRORANGE_250:
        return 250;
    case HwPikoBLX::GYRORANGE_500:
        return 500;
    case HwPikoBLX::GYRORANGE_1000:
        return 1000;
    case HwPikoBLX::GYRORANGE_2000:
        return 2000;
    default:
        break;
    }

    return 500;
}

QStringList PikoBLX::getAdcNames()
{
    return QStringList() << "I"
                         << "V";
}

bool PikoBLX::hasAnnunciator(AnnunciatorType annunc)
{
    switch (annunc) {
    case ANNUNCIATOR_HEARTBEAT:
    case ANNUNCIATOR_ALARM:
    case ANNUNCIATOR_BUZZER:
        return true;
    case ANNUNCIATOR_RGB:
    case ANNUNCIATOR_DAC:
        break;
    }
    return false;
}
