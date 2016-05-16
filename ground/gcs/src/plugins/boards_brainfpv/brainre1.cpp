/**
 ******************************************************************************
 * @file       brainre1.cpp
 * @author     BrainFPV 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_BrainFPVPlugin BrainFPV boards support Plugin
 * @{
 * @brief Plugin to support boards by BrainFPV
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

#include "brainre1.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>

#include "brainre1configuration.h"
#include "hwbrainre1.h"

/**
 * @brief Brain::Brain
 *  This is the Brain board definition
 */
BrainRE1::BrainRE1(void)
{
    // Initialize our USB Structure definition here:
    USBInfo board;
    board.vendorID = 0x20A0;
    board.productID = 0x4242;

    setUSBInfo(board);

    boardType = 0x8B;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(4);
    channelBanks[0] = QVector<int> () << 1 << 2 << 3 << 4; // TIM5
    channelBanks[1] = QVector<int> () << 5; // TIM1
    channelBanks[2] = QVector<int> () << 6; // TIM2
    channelBanks[3] = QVector<int> () << 7 << 8; // TIM8
}

BrainRE1::~BrainRE1()
{

}


QString BrainRE1::shortName()
{
    return QString("BrainRE1");
}

QString BrainRE1::boardDescription()
{
    return QString("BrainFPV RE1 - FPV Racing Flight Controller");
}

//! Return which capabilities this board has
bool BrainRE1::queryCapabilities(BoardCapabilities capability)
{
    switch(capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_OSD:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    default:
        return false;
    }
    return false;
}


/**
 * @brief Brain::getSupportedProtocols
 *  TODO: this is just a stub, we'll need to extend this a lot with multi protocol support
 * @return
 */
QStringList BrainRE1::getSupportedProtocols()
{

    return QStringList("uavtalk");
}

QPixmap BrainRE1::getBoardPicture()
{
	return QPixmap(":/brainfpv/images/brainre1.png");
}

//! Determine if this board supports configuring the receiver
bool BrainRE1::isInputConfigurationSupported(enum InputType type = INPUT_TYPE_ANY)
{
    switch (type) {
    case INPUT_TYPE_PWM:
        return false;
    default:
        return true;
    }
}

QString BrainRE1::getHwUAVO()
{
    return "HwBrainRE1";
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @param port_num which input port to configure (board specific numbering)
 * @return true if successfully configured or false otherwise
 */
bool BrainRE1::setInputType(enum InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwBrainRE1 *hwBrainRE1 = HwBrainRE1::GetInstance(uavoManager);
    Q_ASSERT(hwBrainRE1);
    if (!hwBrainRE1)
        return false;

    HwBrainRE1::DataFields settings = hwBrainRE1->getData();

    switch(type) {
    case INPUT_TYPE_PPM:
        settings.RxPort = HwBrainRE1::RXPORT_PPM;
        break;
    case INPUT_TYPE_SBUS:
        settings.RxPort = HwBrainRE1::RXPORT_SBUS;
        break;
    case INPUT_TYPE_DSM:
        settings.RxPort = HwBrainRE1::RXPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.RxPort = HwBrainRE1::RXPORT_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.RxPort = HwBrainRE1::RXPORT_HOTTSUMH;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwBrainRE1->setData(settings);

    return true;
}

/**
 * @brief Brain::getInputOnPort fetch the currently selected input type
 * @param port_num the port number to query (must be zero)
 * @return the selected input type
 */
enum Core::IBoardType::InputType BrainRE1::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwBrainRE1 *hwBrainRE1 = HwBrainRE1::GetInstance(uavoManager);
    Q_ASSERT(hwBrainRE1);
    if (!hwBrainRE1)
        return INPUT_TYPE_UNKNOWN;

    HwBrainRE1::DataFields settings = hwBrainRE1->getData();

    switch(settings.RxPort) {
        case HwBrainRE1::RXPORT_PPM:
            return INPUT_TYPE_PPM;
        case HwBrainRE1::RXPORT_SBUS:
            return INPUT_TYPE_SBUS;
        case HwBrainRE1::RXPORT_DSM:
            return INPUT_TYPE_DSM;
        case HwBrainRE1::RXPORT_HOTTSUMD:
            return INPUT_TYPE_HOTTSUMD;
        case HwBrainRE1::RXPORT_HOTTSUMH:
            return INPUT_TYPE_HOTTSUMH;
    }

    return INPUT_TYPE_UNKNOWN;
}

int BrainRE1::queryMaxGyroRate()
{
    return 2000;
}

QStringList BrainRE1::getAdcNames()
{
    return QStringList() << "V" << "I" << "R";
}

QWidget * BrainRE1::getBoardConfiguration(QWidget *parent, bool connected)
{
	Q_UNUSED(connected);
	return new BrainRE1Configuration(parent);
}
