/**
 ******************************************************************************
 *
 * @file       acbro.cpp
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

#include "acbro.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>

#include "hwacbro.h"

/**
 * @brief AcBro:AcBro
 *  This is the AcBro board definition
 */
AcBro::AcBro(void)
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

AcBro::~AcBro()
{

}

QString AcBro::shortName()
{
    return QString("AcBro");
}

QString AcBro::boardDescription()
{
    return QString("Brotronics AcBro hardware by ReadError.");
}

//! Return which capabilities this board has
bool AcBro::queryCapabilities(BoardCapabilities capability)
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
    }
    return false;
}


/**
 * @brief AcBro::getSupportedProtocols
 *  TODO: this is just a stub, we'll need to extend this a lot with multi protocol support
 * @return
 */
QStringList AcBro::getSupportedProtocols()
{

    return QStringList("uavtalk");
}

QPixmap AcBro::getBoardPicture()
{
    return QPixmap(":/brotronics/images/acbro.png");
}

QString AcBro::getHwUAVO()
{
    return "HwAcBro";
}

//! Determine if this board supports configuring the receiver
bool AcBro::isInputConfigurationSupported()
{
    return false;
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @param port_num which input port to configure (board specific numbering)
 * @return true if successfully configured or false otherwise
 */
bool AcBro::setInputOnPort(enum InputType type, int port_num)
{
    if (port_num != 0)
        return false;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwAcBro *hwAcBro = HwAcBro::GetInstance(uavoManager);
    Q_ASSERT(hwAcBro);
    if (!hwAcBro)
        return false;

    HwAcBro::DataFields settings = hwAcBro->getData();

    switch(type) {
    case INPUT_TYPE_PPM:
        settings.RcvrPort = HwAcBro::RCVRPORT_PPM;
        break;
    case INPUT_TYPE_SBUS:
        settings.RcvrPort = HwAcBro::RCVRPORT_SBUS;
        break;
    case INPUT_TYPE_DSM:
        settings.RcvrPort = HwAcBro::RCVRPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.RcvrPort = HwAcBro::RCVRPORT_HOTTSUMD;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwAcBro->setData(settings);

    return true;
}

/**
 * @brief AcBro::getInputOnPort fetch the currently selected input type
 * @param port_num the port number to query (must be zero)
 * @return the selected input type
 */
enum Core::IBoardType::InputType AcBro::getInputOnPort(int port_num)
{
    if (port_num != 0)
        return INPUT_TYPE_UNKNOWN;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwAcBro *hwAcBro = HwAcBro::GetInstance(uavoManager);
    Q_ASSERT(hwAcBro);
    if (!hwAcBro)
        return INPUT_TYPE_UNKNOWN;

    HwAcBro::DataFields settings = hwAcBro->getData();

    switch(settings.RcvrPort) {
    case HwAcBro::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwAcBro::RCVRPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwAcBro::RCVRPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwAcBro::RCVRPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    default:
        return INPUT_TYPE_UNKNOWN;
    }
}

int AcBro::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwAcBro *hwAcBro = HwAcBro::GetInstance(uavoManager);
    Q_ASSERT(hwAcBro);
    if (!hwAcBro)
        return 0;

    HwAcBro::DataFields settings = hwAcBro->getData();

    switch(settings.GyroRange) {
    case HwAcBro::GYRORANGE_250:
        return 250;
    case HwAcBro::GYRORANGE_500:
        return 500;
    case HwAcBro::GYRORANGE_1000:
        return 1000;
    case HwAcBro::GYRORANGE_2000:
        return 2000;
    default:
        return 500;
    }
}
