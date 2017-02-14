/**
 ******************************************************************************
 *
 * @file       dtfc.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_DTF DTF boards support Plugin
 * @{
 * @brief Plugin to support DTF boards
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "dtfc.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "hwdtfc.h"
#include "dtfcconfiguration.h"

/**
 * @brief Dtfc:Dtfc
 *  This is the DTFc board definition
 */
Dtfc::Dtfc(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));
    // Legacy USB IDs
    addBootloaderUSBInfo(USBInfo(DRONIN_VID_DTFAIR_DTFC, DRONIN_PID_DTFAIR_DTFC, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(USBInfo(DRONIN_VID_DTFAIR_DTFC, DRONIN_PID_DTFAIR_DTFC, BCD_DEVICE_FIRMWARE));

    boardType = 0xD7;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(3);
    channelBanks[0] = QVector<int> () << 1 << 2; // TIM4
    channelBanks[1] = QVector<int> () << 3 << 4 << 5 << 6; // TIM2
    channelBanks[2] = QVector<int> () << 7 << 8; // TIM3
}

Dtfc::~Dtfc()
{

}

QString Dtfc::shortName()
{
    return QString("DTFc");
}

QString Dtfc::boardDescription()
{
    return QString("DTFc Flight Controller");
}

//! Return which capabilities this board has
bool Dtfc::queryCapabilities(BoardCapabilities capability)
{
    switch(capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_BAROS:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    default:
        break;
    }
    
    return false;
}

QPixmap Dtfc::getBoardPicture()
{
    return QPixmap(":/dtf/images/dtfc.png");
}

QString Dtfc::getHwUAVO()
{
    return "HwDtfc";
}

//! Determine if this board supports configuring the receiver
bool Dtfc::isInputConfigurationSupported(Core::IBoardType::InputType type)
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
bool Dtfc::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwDtfc *hwDtfc = HwDtfc::GetInstance(uavoManager);
    Q_ASSERT(hwDtfc);
    if (!hwDtfc)
        return false;

    HwDtfc::DataFields settings = hwDtfc->getData();

    switch(type) {
    case INPUT_TYPE_PPM:
        settings.RcvrPort = HwDtfc::RCVRPORT_PPM;
        break;
    case INPUT_TYPE_SBUS:
        settings.RcvrPort = HwDtfc::RCVRPORT_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.RcvrPort = HwDtfc::RCVRPORT_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_DSM:
        settings.RcvrPort = HwDtfc::RCVRPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.RcvrPort = HwDtfc::RCVRPORT_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.RcvrPort = HwDtfc::RCVRPORT_HOTTSUMH;
        break;
    case INPUT_TYPE_IBUS:
        settings.RcvrPort = HwDtfc::RCVRPORT_IBUS;
        break;
    case INPUT_TYPE_SRXL:
        settings.RcvrPort = HwDtfc::RCVRPORT_SRXL;
        break;
    default:
        return false;
    }

    hwDtfc->setData(settings);

    return true;
}

/**
 * @brief Dtfc::getInputOnPort fetch the currently selected input type
 * @return the selected input type
 */
Core::IBoardType::InputType Dtfc::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwDtfc *hwDtfc = HwDtfc::GetInstance(uavoManager);
    Q_ASSERT(hwDtfc);
    if (!hwDtfc)
        return INPUT_TYPE_UNKNOWN;

    HwDtfc::DataFields settings = hwDtfc->getData();

    switch(settings.RcvrPort) {
    case HwDtfc::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwDtfc::RCVRPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwDtfc::RCVRPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwDtfc::RCVRPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwDtfc::RCVRPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwDtfc::RCVRPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwDtfc::RCVRPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwDtfc::RCVRPORT_SRXL:
        return INPUT_TYPE_SRXL;
    default:
        break;
    }
    
    switch(settings.Uart1) {
    case HwDtfc::UART1_SBUS:
        return INPUT_TYPE_SBUS;
    case HwDtfc::UART1_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwDtfc::UART1_DSM:
        return INPUT_TYPE_DSM;
    case HwDtfc::UART1_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwDtfc::UART1_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwDtfc::UART1_IBUS:
        return INPUT_TYPE_IBUS;
    case HwDtfc::UART1_SRXL:
        return INPUT_TYPE_SRXL;
    default:
        break;
    }
    
    switch(settings.Uart2) {
    case HwDtfc::UART2_SBUS:
        return INPUT_TYPE_SBUS;
    case HwDtfc::UART2_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwDtfc::UART2_DSM:
        return INPUT_TYPE_DSM;
    case HwDtfc::UART2_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwDtfc::UART2_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwDtfc::UART2_IBUS:
        return INPUT_TYPE_IBUS;
    case HwDtfc::UART2_SRXL:
        return INPUT_TYPE_SRXL;
    default:
        break;
    }
    
    return INPUT_TYPE_UNKNOWN;
}

int Dtfc::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwDtfc *hwDtfc = HwDtfc::GetInstance(uavoManager);
    Q_ASSERT(hwDtfc);
    if (!hwDtfc)
        return 0;

    HwDtfc::DataFields settings = hwDtfc->getData();

    switch(settings.GyroRange) {
    case HwDtfc::GYRORANGE_250:
        return 250;
    case HwDtfc::GYRORANGE_500:
        return 500;
    case HwDtfc::GYRORANGE_1000:
        return 1000;
    case HwDtfc::GYRORANGE_2000:
        return 2000;
    default:
        break;
    }
    
    return 500;
}

QStringList Dtfc::getAdcNames()
{
    return QStringList() << "Current" << "Battery";
}

QWidget *Dtfc::getBoardConfiguration(QWidget *parent, bool connected)
{
    Q_UNUSED(connected);
    return new DtfcConfiguration(parent);
}
