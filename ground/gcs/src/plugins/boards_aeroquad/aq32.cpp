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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "aq32.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>

#include "hwaq32.h"

/**
 * @brief AQ32::AQ32
 *  This is the AQ32 board definition
 */
AQ32::AQ32(void)
{
    // Initialize our USB Structure definition here:
    USBInfo board;
    board.vendorID  = 0x20a0;
    board.productID = 0x4284;

    setUSBInfo(board);

    boardType = 0x94;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(5);
    channelBanks[0] = QVector<int> () << 1 << 2;
    channelBanks[1] = QVector<int> () << 3 << 4;
    channelBanks[2] = QVector<int> () << 5 << 6;
    channelBanks[3] = QVector<int> () << 7 << 8 << 9 <<10;
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
    switch(capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_MAGS:
    case BOARD_CAPABILITIES_BAROS:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    default:
        return false;
    }
    return false;
}


/**
 * @brief AQ32::getSupportedProtocols
 *  TODO: this is just a stub, we'll need to extend this a lot with multi protocol support
 * @return
 */
QStringList AQ32::getSupportedProtocols()
{

    return QStringList("uavtalk");
}

QPixmap AQ32::getBoardPicture()
{
    return QPixmap(":/aq32/images/aq32.png");
}


//! Determine if this board supports configuring the receiver
bool AQ32::isInputConfigurationSupported(enum InputType type = INPUT_TYPE_ANY)
{
    Q_UNUSED(type);
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
bool AQ32::setInputType(enum InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwAQ32 *hwAQ32 = HwAQ32::GetInstance(uavoManager);
    Q_ASSERT(hwAQ32);
    if (!hwAQ32)
        return false;

    HwAQ32::DataFields settings = hwAQ32->getData();

    switch(type) {
    case INPUT_TYPE_PPM:
        settings.RcvrPort = HwAQ32::RCVRPORT_PPM;
        break;
    case INPUT_TYPE_PWM:
        settings.RcvrPort = HwAQ32::RCVRPORT_PWM;
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
    case INPUT_TYPE_IBUS: // Is not selectable yet in the Vehicle Setup Wizard, but if it ends up there, this is already in place.
        settings.Uart3 = HwAQ32::UART3_IBUS;
        break;
    case INPUT_TYPE_DSM:
        settings.Uart4 = HwAQ32::UART4_DSM;
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
enum Core::IBoardType::InputType AQ32::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwAQ32 *hwAQ32 = HwAQ32::GetInstance(uavoManager);
    Q_ASSERT(hwAQ32);
    if (!hwAQ32)
        return INPUT_TYPE_UNKNOWN;

    HwAQ32::DataFields settings = hwAQ32->getData();

    switch(settings.RcvrPort) {
    case HwAQ32::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwAQ32::RCVRPORT_PWM:
        return INPUT_TYPE_PWM;
    default:
        break;
    }
    
    switch(settings.Uart3) {
    case HwAQ32::UART3_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwAQ32::UART3_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwAQ32::UART3_SBUS:
        return INPUT_TYPE_SBUS;
    case HwAQ32::UART3_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;    
    case HwAQ32::UART3_IBUS: // None of the other targets have IBUS in getInputType, but seems to be no problem.
        return INPUT_TYPE_IBUS;
    }

    switch(settings.Uart4) {
    case HwAQ32::UART4_DSM:
        return INPUT_TYPE_DSM;
    case HwAQ32::UART4_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwAQ32::UART4_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwAQ32::UART4_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;    
    case HwAQ32::UART4_IBUS: // None of the other targets have IBUS in getInputType, but seems to be no problem.
        return INPUT_TYPE_IBUS;
    }

    switch(settings.Uart6) {
    case HwAQ32::UART6_DSM:
        return INPUT_TYPE_DSM;
    case HwAQ32::UART6_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwAQ32::UART6_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwAQ32::UART6_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;    
    case HwAQ32::UART6_IBUS: // None of the other targets have IBUS in getInputType, but seems to be no problem.
        return INPUT_TYPE_IBUS;
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

    switch(settings.GyroRange) {
    case HwAQ32::GYRORANGE_250:
        return 250;
    case HwAQ32::GYRORANGE_500:
        return 500;
    case HwAQ32::GYRORANGE_1000:
        return 1000;
    case HwAQ32::GYRORANGE_2000:
        return 2000;
    default:
        return 500;
    }
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
        return QStringList() << "AI2" << "BM" << "AI3" << "AI4" << "AI5";
    }

    return QStringList() << "Disabled" << "Disabled" << "Disabled" << "Disabled" << "Disabled";
}
