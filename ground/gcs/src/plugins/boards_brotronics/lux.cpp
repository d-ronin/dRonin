/**
 ******************************************************************************
 *
 * @file       lux.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "lux.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>

#include "hwlux.h"

/**
 * @brief Lux:Lux
 *  This is the Lux board definition
 */
Lux::Lux(void)
{
    // Initialize our USB Structure definition here:
    USBInfo board;
    board.vendorID = 0x20A0;
    board.productID = 0x4195;
    setUSBInfo(board);

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
        return true;
    case BOARD_CAPABILITIES_ACCELS:
        return true;
    case BOARD_CAPABILITIES_MAGS:
        return false;
    case BOARD_CAPABILITIES_BAROS:
        return false;
    case BOARD_CAPABILITIES_RADIO:
        return false;
    case BOARD_CAPABILITIES_OSD:
        return false;
    }
    return false;
}


/**
 * @brief Lux::getSupportedProtocols
 *  TODO: this is just a stub, we'll need to extend this a lot with multi protocol support
 * @return
 */
QStringList Lux::getSupportedProtocols()
{

    return QStringList("uavtalk");
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
bool Lux::isInputConfigurationSupported()
{
    return false;
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @param port_num which input port to configure (board specific numbering)
 * @return true if successfully configured or false otherwise
 */
bool Lux::setInputOnPort(enum InputType type, int port_num)
{
    if (port_num != 0)
        return false;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwLux *hwLux = HwLux::GetInstance(uavoManager);
    Q_ASSERT(hwLux);
    if (!hwLux)
        return false;

    HwLux::DataFields settings = hwLux->getData();

    switch(type) {
    case INPUT_TYPE_PPM:
        settings.RcvrPort = HwLux::RCVRPORT_PPM;
        break;
    case INPUT_TYPE_SBUS:
        settings.RcvrPort = HwLux::RCVRPORT_SBUS;
        break;
    case INPUT_TYPE_DSM:
        settings.RcvrPort = HwLux::RCVRPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.RcvrPort = HwLux::RCVRPORT_HOTTSUMD;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwLux->setData(settings);

    return true;
}

/**
 * @brief Lux::getInputOnPort fetch the currently selected input type
 * @param port_num the port number to query (must be zero)
 * @return the selected input type
 */
enum Core::IBoardType::InputType Lux::getInputOnPort(int port_num)
{
    if (port_num != 0)
        return INPUT_TYPE_UNKNOWN;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwLux *hwLux = HwLux::GetInstance(uavoManager);
    Q_ASSERT(hwLux);
    if (!hwLux)
        return INPUT_TYPE_UNKNOWN;

    HwLux::DataFields settings = hwLux->getData();

    switch(settings.RcvrPort) {
    case HwLux::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwLux::RCVRPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwLux::RCVRPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwLux::RCVRPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    default:
        return INPUT_TYPE_UNKNOWN;
    }
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
        return 500;
    }
}
