/**
 ******************************************************************************
 *
 * @file       quanton.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_Quantec Quantec boards support Plugin
 * @{
 * @brief Plugin to support boards by Quantec
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

#include "quanton.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>

#include "hwquanton.h"

/**
 * @brief Quanton::Quanton
 *  This is the Quanton board definition
 */
Quanton::Quanton(void)
{
    // Initialize our USB Structure definition here:
    USBInfo board;
    board.vendorID = 0x0fda;
    board.productID = 0x0100;

    setUSBInfo(board);

    boardType = 0x86;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(6);
    channelBanks[0] = QVector<int> () << 1 << 2 << 3 << 4;
    channelBanks[1] = QVector<int> () << 5 << 6;
    channelBanks[2] = QVector<int> () << 7;
    channelBanks[3] = QVector<int> () << 8;
}

Quanton::~Quanton()
{

}

QString Quanton::shortName()
{
    return QString("quanton");
}

QString Quanton::boardDescription()
{
    return QString("quanton flight control rev. 1 by Quantec Networks GmbH");
}

int Quanton::minBootLoaderVersion()
{
    return 0x84;
}

//! Return which capabilities this board has
bool Quanton::queryCapabilities(BoardCapabilities capability)
{
    switch(capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_MAGS:
    case BOARD_CAPABILITIES_BAROS:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    }
    
    return false;
}

QPixmap Quanton::getBoardPicture()
{
    return QPixmap(":/quantec/images/quanton.png");
}

//! Determine if this board supports configuring the receiver
bool Quanton::isInputConfigurationSupported(Core::IBoardType::InputType type)
{
    switch (type) {
    case INPUT_TYPE_UNKNOWN:
        return false;
    }
    
    return true;
}

QString Quanton::getHwUAVO()
{
    return "HwQuanton";
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @return true if successfully configured or false otherwise
 */
bool Quanton::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwQuanton *HwQuanton = HwQuanton::GetInstance(uavoManager);
    Q_ASSERT(HwQuanton);
    if (!HwQuanton)
        return false;

    HwQuanton::DataFields settings = HwQuanton->getData();

    switch(type) {
    case INPUT_TYPE_PPM:
        settings.InPort = HwQuanton::INPORT_PPM;
        break;
    case INPUT_TYPE_PWM:
        settings.InPort = HwQuanton::INPORT_PWM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.Uart1 = HwQuanton::UART1_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.Uart1 = HwQuanton::UART1_HOTTSUMD;
        break;
    case INPUT_TYPE_SBUS:
        settings.Uart2 = HwQuanton::UART2_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.Uart1 = HwQuanton::UART1_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_IBUS:
        settings.Uart1 = HwQuanton::UART1_IBUS;
        break;
    case INPUT_TYPE_SRXL:
        settings.Uart1 = HwQuanton::UART1_SRXL;
        break;
    case INPUT_TYPE_DSM:
        settings.Uart1 = HwQuanton::UART1_DSM;
        break;    
    default:
        return false;
    }

    // Apply these changes
    HwQuanton->setData(settings);

    return true;
}

/**
 * @brief Quanton::getInputType fetch the currently selected input type
 * @return the selected input type
 */
Core::IBoardType::InputType Quanton::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwQuanton *HwQuanton = HwQuanton::GetInstance(uavoManager);
    Q_ASSERT(HwQuanton);
    if (!HwQuanton)
        return INPUT_TYPE_UNKNOWN;

    HwQuanton::DataFields settings = HwQuanton->getData();

    switch(settings.InPort) {
    case HwQuanton::INPORT_PPM:
    case HwQuanton::INPORT_PPMADC:
    case HwQuanton::INPORT_PPMOUTPUTS:
    case HwQuanton::INPORT_PPMOUTPUTSADC:
    // discutable, are these PPM, PWM or defined by the InPortSerial port?
    // For now defined as PPM.
    case HwQuanton::INPORT_PPMSERIAL:
    case HwQuanton::INPORT_PPMSERIALADC:
    case HwQuanton::INPORT_PPMPWM:
    case HwQuanton::INPORT_PPMPWMADC:
        return INPUT_TYPE_PPM;
    case HwQuanton::INPORT_PWM:
    case HwQuanton::INPORT_PWMADC:
        return INPUT_TYPE_PWM;
    case HwQuanton::INPORT_SERIAL:
        switch(settings.InPortSerial) {
        case HwQuanton::INPORTSERIAL_DSM:
            return INPUT_TYPE_DSM;
        case HwQuanton::INPORTSERIAL_HOTTSUMD:
            return INPUT_TYPE_HOTTSUMD;
        case HwQuanton::INPORTSERIAL_HOTTSUMH:
            return INPUT_TYPE_HOTTSUMH;
        case HwQuanton::INPORTSERIAL_SBUSNONINVERTED:
            return INPUT_TYPE_SBUSNONINVERTED;
        case HwQuanton::INPORTSERIAL_IBUS:
            return INPUT_TYPE_IBUS;
        case HwQuanton::INPORTSERIAL_SRXL:
            return INPUT_TYPE_SRXL;
        }
        break;
    }
    
    switch(settings.Uart1) {
    case HwQuanton::UART1_DSM:
        return INPUT_TYPE_DSM;
    case HwQuanton::UART1_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwQuanton::UART1_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwQuanton::UART1_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwQuanton::UART1_IBUS:
        return INPUT_TYPE_IBUS;
    case HwQuanton::UART1_SRXL:
        return INPUT_TYPE_SRXL;
    }

    switch(settings.Uart2) {
    case HwQuanton::UART2_DSM:
        return INPUT_TYPE_DSM;
    case HwQuanton::UART2_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwQuanton::UART2_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwQuanton::UART2_SBUS:
        return INPUT_TYPE_SBUS;
    case HwQuanton::UART2_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwQuanton::UART2_IBUS:
        return INPUT_TYPE_IBUS;
    case HwQuanton::UART2_SRXL:
        return INPUT_TYPE_SRXL;
    }

    switch(settings.Uart3) {
    case HwQuanton::UART3_DSM:
        return INPUT_TYPE_DSM;
    case HwQuanton::UART3_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwQuanton::UART3_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwQuanton::UART3_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwQuanton::UART3_IBUS:
        return INPUT_TYPE_IBUS;
    case HwQuanton::UART3_SRXL:
        return INPUT_TYPE_SRXL;
    }
    
    switch(settings.Uart4) {
    case HwQuanton::UART4_DSM:
        return INPUT_TYPE_DSM;
    case HwQuanton::UART4_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwQuanton::UART4_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwQuanton::UART4_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwQuanton::UART4_IBUS:
        return INPUT_TYPE_IBUS;
    case HwQuanton::UART4_SRXL:
        return INPUT_TYPE_SRXL;
    }
    
    switch(settings.Uart5) {
    case HwQuanton::UART5_DSM:
        return INPUT_TYPE_DSM;
    case HwQuanton::UART5_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwQuanton::UART5_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwQuanton::UART5_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwQuanton::UART5_IBUS:
        return INPUT_TYPE_IBUS;
    case HwQuanton::UART5_SRXL:
        return INPUT_TYPE_SRXL;
    }

    return INPUT_TYPE_UNKNOWN;
}

int Quanton::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwQuanton *hwQuanton = HwQuanton::GetInstance(uavoManager);
    Q_ASSERT(hwQuanton);
    if (!hwQuanton)
        return 0;

    HwQuanton::DataFields settings = hwQuanton->getData();

    switch(settings.GyroRange) {
    case HwQuanton::GYRORANGE_250:
        return 250;
    case HwQuanton::GYRORANGE_500:
        return 500;
    case HwQuanton::GYRORANGE_1000:
        return 1000;
    case HwQuanton::GYRORANGE_2000:
        return 2000;
    }
    
    return 500;
}

QStringList Quanton::getAdcNames()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwQuanton *hwQuanton = HwQuanton::GetInstance(uavoManager);
    Q_ASSERT(hwQuanton);
    if (!hwQuanton)
        return QStringList();

    HwQuanton::DataFields settings = hwQuanton->getData();
    if (settings.InPort == HwQuanton::INPORT_OUTPUTSADC ||
            settings.InPort == HwQuanton::INPORT_PPMADC ||
            settings.InPort == HwQuanton::INPORT_PPMOUTPUTSADC ||
            settings.InPort == HwQuanton::INPORT_PPMPWMADC ||
            settings.InPort == HwQuanton::INPORT_PWMADC ||
            settings.InPort == HwQuanton::INPORT_PPMSERIALADC) {
        return QStringList() << "IN 7" << "IN 8";
    }

    return QStringList() << "Disabled" << "Disabled";
}
