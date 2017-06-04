/**
 ******************************************************************************
 * @file       Brain.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_BrainFPV BrainFPV boards support Plugin
 * @{
 * @brief Plugin to support boards by BrainFPV LLC
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

#include "brain.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "brainconfiguration.h"
#include "hwbrain.h"

/**
 * @brief Brain::Brain
 *  This is the Brain board definition
 */
Brain::Brain(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));
    // Legacy USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_BRAINFPV_BRAIN, DRONIN_PID_BRAINFPV_BRAIN, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_BRAINFPV_BRAIN, DRONIN_PID_BRAINFPV_BRAIN, BCD_DEVICE_FIRMWARE));

    boardType = 0x8A;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(3);
    channelBanks[0] = QVector<int>() << 1 << 2 << 3 << 4; // TIM5 main outputs
    channelBanks[1] = QVector<int>() << 5 << 6 << 7 << 8; // TIM8 on receiverport
    channelBanks[2] = QVector<int>() << 9 << 10; // TIM12 on receiverport
}

Brain::~Brain()
{
}

int Brain::minBootLoaderVersion()
{
    return 0x82;
}

QString Brain::shortName()
{
    return QString("Brain");
}

QString Brain::boardDescription()
{
    return QString("Brain - FPV Flight Controller");
}

//! Return which capabilities this board has
bool Brain::queryCapabilities(BoardCapabilities capability)
{
    switch (capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_MAGS:
    case BOARD_CAPABILITIES_BAROS:
    case BOARD_CAPABILITIES_OSD:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    default:
        break;
    }

    return false;
}

QPixmap Brain::getBoardPicture()
{
    return QPixmap(":/brainfpv/images/brain.png");
}

QString Brain::getHwUAVO()
{
    return "HwBrain";
}

//! Determine if this board supports configuring the receiver
bool Brain::isInputConfigurationSupported(Core::IBoardType::InputType type)
{
    switch (type) {
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
bool Brain::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwBrain *hwBrain = HwBrain::GetInstance(uavoManager);
    Q_ASSERT(hwBrain);
    if (!hwBrain)
        return false;

    HwBrain::DataFields settings = hwBrain->getData();

    switch (type) {
    case INPUT_TYPE_PPM:
        settings.RxPort = HwBrain::RXPORT_PPM;
        break;
    case INPUT_TYPE_PWM:
        settings.RxPort = HwBrain::RXPORT_PWM;
        break;
    case INPUT_TYPE_SBUS:
        settings.MainPort = HwBrain::MAINPORT_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.MainPort = HwBrain::MAINPORT_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_DSM:
        settings.MainPort = HwBrain::MAINPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.MainPort = HwBrain::MAINPORT_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.MainPort = HwBrain::MAINPORT_HOTTSUMH;
        break;
    case INPUT_TYPE_IBUS:
        settings.MainPort = HwBrain::MAINPORT_IBUS;
        break;
    case INPUT_TYPE_SRXL:
        settings.MainPort = HwBrain::MAINPORT_SRXL;
        break;
    case INPUT_TYPE_TBSCROSSFIRE:
        settings.MainPort = HwBrain::MAINPORT_TBSCROSSFIRE;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwBrain->setData(settings);

    return true;
}

/**
 * @brief Brain::getInputType fetch the currently selected input type
 * @return the selected input type
 */
Core::IBoardType::InputType Brain::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwBrain *hwBrain = HwBrain::GetInstance(uavoManager);
    Q_ASSERT(hwBrain);
    if (!hwBrain)
        return INPUT_TYPE_UNKNOWN;

    HwBrain::DataFields settings = hwBrain->getData();

    switch (settings.MainPort) {
    case HwBrain::MAINPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwBrain::MAINPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwBrain::MAINPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwBrain::MAINPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwBrain::MAINPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwBrain::MAINPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwBrain::MAINPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwBrain::MAINPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.FlxPort) {
    case HwBrain::FLXPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwBrain::FLXPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwBrain::FLXPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwBrain::FLXPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwBrain::FLXPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwBrain::FLXPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.RxPort) {
    case HwBrain::RXPORT_PPM:
    case HwBrain::RXPORT_PPMPWM:
    case HwBrain::RXPORT_PPMOUTPUTS:
    case HwBrain::RXPORT_PPMUART:
    case HwBrain::RXPORT_PPMUARTOUTPUTS:
    case HwBrain::RXPORT_PPMFRSKY:
        return INPUT_TYPE_PPM;
    case HwBrain::RXPORT_PWM:
        return INPUT_TYPE_PWM;
    case HwBrain::RXPORT_UART:
    case HwBrain::RXPORT_UARTOUTPUTS:
        switch (settings.RxPortUsart) {
        case HwBrain::RXPORTUSART_DSM:
            return INPUT_TYPE_DSM;
        case HwBrain::RXPORTUSART_HOTTSUMD:
            return INPUT_TYPE_HOTTSUMD;
        case HwBrain::RXPORTUSART_HOTTSUMH:
            return INPUT_TYPE_HOTTSUMH;
        case HwBrain::RXPORTUSART_SBUSNONINVERTED:
            return INPUT_TYPE_SBUSNONINVERTED;
        case HwBrain::RXPORTUSART_IBUS:
            return INPUT_TYPE_IBUS;
        case HwBrain::RXPORTUSART_SRXL:
            return INPUT_TYPE_SRXL;
        case HwBrain::RXPORTUSART_TBSCROSSFIRE:
            return INPUT_TYPE_TBSCROSSFIRE;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return INPUT_TYPE_UNKNOWN;
}

int Brain::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwBrain *hwBrain = HwBrain::GetInstance(uavoManager);
    Q_ASSERT(hwBrain);
    if (!hwBrain)
        return 0;

    HwBrain::DataFields settings = hwBrain->getData();

    switch (settings.GyroFullScale) {
    case HwBrain::GYROFULLSCALE_250:
        return 250;
    case HwBrain::GYROFULLSCALE_500:
        return 500;
    case HwBrain::GYROFULLSCALE_1000:
        return 1000;
    case HwBrain::GYROFULLSCALE_2000:
        return 2000;
    default:
        break;
    }

    return 2000;
}

QWidget *Brain::getBoardConfiguration(QWidget *parent, bool connected)
{
    Q_UNUSED(connected);
    return new BrainConfiguration(parent);
}

QStringList Brain::getAdcNames()
{
    return QStringList() << "Sen ADC0"
                         << "Sen ADC1"
                         << "Sen ADC2";
}

bool Brain::hasAnnunciator(AnnunciatorType annunc)
{
    switch (annunc) {
    case ANNUNCIATOR_HEARTBEAT:
    case ANNUNCIATOR_ALARM:
    case ANNUNCIATOR_DAC:
        return true;
    case ANNUNCIATOR_BUZZER:
    case ANNUNCIATOR_RGB:
        break;
    }
    return false;
}
