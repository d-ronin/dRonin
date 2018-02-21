/**
 ******************************************************************************
 * @file       naze32pro.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "naze32pro.h"

#include "uavobjects/uavobjectmanager.h"
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "hwnaze32pro.h"

/**
 * @brief Naze32Pro::Naze32Pro
 *  This is the Naze32Pro board definition
 */
Naze32Pro::Naze32Pro(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));

	boardType = 0x93;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(4);
    channelBanks[0] = QVector<int> () << 1 << 2;
    channelBanks[1] = QVector<int> () << 3 << 4;
    channelBanks[2] = QVector<int> () << 5 << 6;
    channelBanks[3] = QVector<int> () << 7 << 8;
}

Naze32Pro::~Naze32Pro()
{

}

QString Naze32Pro::shortName()
{
    return QString("Naze32Pro");
}

QString Naze32Pro::boardDescription()
{
    return QString("The Naze32Pro board");
}

//! Return which capabilities this board has
bool Naze32Pro::queryCapabilities(BoardCapabilities capability)
{
    switch(capability) {
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

/**
 * @brief Naze32Pro::getSupportedProtocols
 *  TODO: this is just a stub, we'll need to extend this a lot with multi protocol support
 * @return
 */
QStringList Naze32Pro::getSupportedProtocols()
{

    return QStringList("uavtalk");
}

QPixmap Naze32Pro::getBoardPicture()
{
    return QPixmap(":/naze/images/naze32pro.png");
}

QString Naze32Pro::getHwUAVO()
{
    return "HwNaze32Pro";
}

//! Determine if this board supports configuring the receiver
bool Naze32Pro::isInputConfigurationSupported()
{
    return true;
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @param port_num which input port to configure (board specific numbering)
 * @return true if successfully configured or false otherwise
 */
bool Naze32Pro::setInputOnPort(enum InputType type, int port_num)
{
    if (port_num != 0)
        return false;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwNaze32Pro *hwNaze32Pro = HwNaze32Pro::GetInstance(uavoManager);
    Q_ASSERT(hwNaze32Pro);
    if (!hwNaze32Pro)
        return false;

    HwNaze32Pro::DataFields settings = hwNaze32Pro->getData();

    switch(type) {
    case INPUT_TYPE_PPM:
        settings.RcvrPort = HwNaze32Pro::RCVRPORT_PPM;
        break;
    case INPUT_TYPE_SBUS:
        settings.RcvrPort = HwNaze32Pro::RCVRPORT_SBUS;
        break;
    case INPUT_TYPE_DSM:
        settings.RcvrPort = HwNaze32Pro::RCVRPORT_DSM;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwNaze32Pro->setData(settings);

    return true;
}

/**
 * @brief Naze32Pro::getInputOnPort fetch the currently selected input type
 * @param port_num the port number to query (must be zero)
 * @return the selected input type
 */
enum Core::IBoardType::InputType Naze32Pro::getInputOnPort(int port_num)
{
    if (port_num != 0)
        return INPUT_TYPE_UNKNOWN;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwNaze32Pro *hwNaze32Pro = HwNaze32Pro::GetInstance(uavoManager);
    Q_ASSERT(hwNaze32Pro);
    if (!hwNaze32Pro)
        return INPUT_TYPE_UNKNOWN;

    HwNaze32Pro::DataFields settings = hwNaze32Pro->getData();

    switch(settings.RcvrPort) {
    case HwNaze32Pro::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwNaze32Pro::RCVRPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwNaze32Pro::RCVRPORT_DSM:
        return INPUT_TYPE_DSM;
    default:
        return INPUT_TYPE_UNKNOWN;
    }
}

int Naze32Pro::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwNaze32Pro *hwNaze32Pro = HwNaze32Pro::GetInstance(uavoManager);
    Q_ASSERT(hwNaze32Pro);
    if (!hwNaze32Pro)
        return 0;

    HwNaze32Pro::DataFields settings = hwNaze32Pro->getData();

    switch(settings.GyroRange) {
    case HwNaze32Pro::GYRORANGE_250:
        return 250;
    case HwNaze32Pro::GYRORANGE_500:
        return 500;
    case HwNaze32Pro::GYRORANGE_1000:
        return 1000;
    case HwNaze32Pro::GYRORANGE_2000:
        return 2000;
    default:
        return 500;
    }
}

QStringList Naze32Pro::getAdcNames()
{
    return QStringList() << "VREF" << "Voltage" << "Current" << "RSSI";
}