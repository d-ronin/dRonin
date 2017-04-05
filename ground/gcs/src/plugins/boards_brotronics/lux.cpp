/**
 ******************************************************************************
 *
 * @file       lux.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_Brotronics Brotronics boards support Plugin
 * @{
 * @brief Plugin to support Brotronics boards
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

#include "lux.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "hwlux.h"
#include "luxconfiguration.h"

/**
 * @brief Lux:Lux
 *  This is the Lux board definition
 */
Lux::Lux(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));
    // Legacy USB IDs
    addBootloaderUSBInfo(USBInfo(DRONIN_VID_LUMENIER_LUX, DRONIN_PID_LUMENIER_LUX, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(USBInfo(DRONIN_VID_LUMENIER_LUX, DRONIN_PID_LUMENIER_LUX, BCD_DEVICE_FIRMWARE));

    boardType = 0xCA;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(1);
    channelBanks[0] = QVector<int> () << 1 << 2 << 3 << 4; // T3
}

Lux::~Lux()
{

}

QString Lux::shortName()
{
    return QString("Lux");
}

QString Lux::boardDescription()
{
    return QString("Lumenier Lux hardware by ReadError.");
}

//! Return which capabilities this board has
bool Lux::queryCapabilities(BoardCapabilities capability)
{
    switch(capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    default:
        break;
    }
    
    return false;
}

QPixmap Lux::getBoardPicture()
{
    return QPixmap(":/brotronics/images/lux.png");
}

QString Lux::getHwUAVO()
{
    return "HwLux";
}

//! Determine if this board supports configuring the receiver
bool Lux::isInputConfigurationSupported(Core::IBoardType::InputType type)
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
bool Lux::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwLux *hwLux = HwLux::GetInstance(uavoManager);
    Q_ASSERT(hwLux);
    if (!hwLux)
        return false;

    HwLux::DataFields settings = hwLux->getData();

    switch(type) {
    case INPUT_TYPE_PPM:
        // always enabled, do nothing
        break;
    case INPUT_TYPE_SBUS:
        settings.RxPort = HwLux::RXPORT_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.RxPort = HwLux::RXPORT_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_DSM:
        settings.RxPort = HwLux::RXPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.RxPort = HwLux::RXPORT_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.RxPort = HwLux::RXPORT_HOTTSUMH;
        break;
    case INPUT_TYPE_IBUS:
        settings.RxPort = HwLux::RXPORT_IBUS;
        break;
    case INPUT_TYPE_SRXL:
        settings.RxPort = HwLux::RXPORT_SRXL;
        break;
    case INPUT_TYPE_TBSCROSSFIRE:
        settings.RxPort = HwLux::RXPORT_TBSCROSSFIRE;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwLux->setData(settings);

    return true;
}

/**
 * @brief Lux::getInputType fetch the currently selected input type
 * @return the selected input type
 */
Core::IBoardType::InputType Lux::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwLux *hwLux = HwLux::GetInstance(uavoManager);
    Q_ASSERT(hwLux);
    if (!hwLux)
        return INPUT_TYPE_UNKNOWN;

    HwLux::DataFields settings = hwLux->getData();

    switch(settings.RxPort) {
    case HwLux::RXPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwLux::RXPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwLux::RXPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwLux::RXPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwLux::RXPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwLux::RXPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwLux::RXPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwLux::RXPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }
    
    switch(settings.Uart2) {
    case HwLux::UART2_DSM:
        return INPUT_TYPE_DSM;
    case HwLux::UART2_SBUS:
        return INPUT_TYPE_SBUS;
    case HwLux::UART2_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwLux::UART2_IBUS:
        return INPUT_TYPE_IBUS;
    case HwLux::UART2_SRXL:
        return INPUT_TYPE_SRXL;
    case HwLux::UART2_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwLux::UART2_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwLux::UART2_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }
    
    switch(settings.Uart3) {
    case HwLux::UART3_DSM:
        return INPUT_TYPE_DSM;
    case HwLux::UART3_SBUS:
        return INPUT_TYPE_SBUS;
    case HwLux::UART3_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwLux::UART3_IBUS:
        return INPUT_TYPE_IBUS;
    case HwLux::UART3_SRXL:
        return INPUT_TYPE_SRXL;
    case HwLux::UART3_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwLux::UART3_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwLux::UART3_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }
    
    return INPUT_TYPE_PPM;
}

int Lux::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwLux *hwLux = HwLux::GetInstance(uavoManager);
    Q_ASSERT(hwLux);
    if (!hwLux)
        return 0;

    HwLux::DataFields settings = hwLux->getData();

    switch(settings.GyroRange) {
    case HwLux::GYRORANGE_250:
        return 250;
    case HwLux::GYRORANGE_500:
        return 500;
    case HwLux::GYRORANGE_1000:
        return 1000;
    case HwLux::GYRORANGE_2000:
        return 2000;
    default:
        break;
    }
    
    return 500;
}

QStringList Lux::getAdcNames()
{
    return QStringList() << "VBAT" << "Current" << "RSSI";
}

QWidget *Lux::getBoardConfiguration(QWidget *parent, bool connected)
{
    Q_UNUSED(connected);
    return new LuxConfiguration(parent);
}
