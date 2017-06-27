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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */

#include "brainre1.h"

#include "uavobjects/uavobjectmanager.h"
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "brainre1configuration.h"
#include "hwbrainre1.h"

/**
 * @brief Brain::Brain
 *  This is the Brain board definition
 */
BrainRE1::BrainRE1(void)
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

    boardType = 0x8B;
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
    switch (capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_OSD:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    default:
        return false;
    }
}

QPixmap BrainRE1::getBoardPicture()
{
    return QPixmap(":/brainfpv/images/brainre1.png");
}

//! Determine if this board supports configuring the receiver
bool BrainRE1::isInputConfigurationSupported(Core::IBoardType::InputType type)
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
bool BrainRE1::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwBrainRE1 *hwBrainRE1 = HwBrainRE1::GetInstance(uavoManager);
    Q_ASSERT(hwBrainRE1);
    if (!hwBrainRE1)
        return false;

    HwBrainRE1::DataFields settings = hwBrainRE1->getData();

    switch (type) {
    case INPUT_TYPE_PPM:
        settings.RxPort = HwBrainRE1::RXPORT_PPM;
        break;
    case INPUT_TYPE_SBUS:
        settings.RxPort = HwBrainRE1::RXPORT_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.RxPort = HwBrainRE1::RXPORT_SBUSNONINVERTED;
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
    case INPUT_TYPE_IBUS:
        settings.RxPort = HwBrainRE1::RXPORT_IBUS;
        break;
    case INPUT_TYPE_SRXL:
        settings.RxPort = HwBrainRE1::RXPORT_SRXL;
        break;
    case INPUT_TYPE_TBSCROSSFIRE:
        settings.RxPort = HwBrainRE1::RXPORT_TBSCROSSFIRE;
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
Core::IBoardType::InputType BrainRE1::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwBrainRE1 *hwBrainRE1 = HwBrainRE1::GetInstance(uavoManager);
    Q_ASSERT(hwBrainRE1);
    if (!hwBrainRE1)
        return INPUT_TYPE_UNKNOWN;

    HwBrainRE1::DataFields settings = hwBrainRE1->getData();

    switch (settings.RxPort) {
    case HwBrainRE1::RXPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwBrainRE1::RXPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwBrainRE1::RXPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwBrainRE1::RXPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwBrainRE1::RXPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwBrainRE1::RXPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwBrainRE1::RXPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwBrainRE1::RXPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwBrainRE1::RXPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.SerialPort) {
    case HwBrainRE1::SERIALPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.MultiPortSerial) {
    case HwBrainRE1::MULTIPORTSERIAL_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }
    
    if (settings.MultiPortMode == HwBrainRE1::MULTIPORTMODE_DUALSERIAL4PWM)
    {
        switch (settings.MultiPortSerial2) {
        case HwBrainRE1::MULTIPORTSERIAL2_TBSCROSSFIRE:
            return INPUT_TYPE_TBSCROSSFIRE;
        default:
            break;
        }
    }

    return INPUT_TYPE_UNKNOWN;
}

int BrainRE1::queryMaxGyroRate()
{
    return 2000;
}

QStringList BrainRE1::getAdcNames()
{
    return QStringList() << "I"
                         << "V"
                         << "R";
}

QWidget *BrainRE1::getBoardConfiguration(QWidget *parent, bool connected)
{
    Q_UNUSED(connected);
    return new BrainRE1Configuration(parent);
}

QVector<QVector<int>> BrainRE1::getChannelBanks()
{
    QVector<QVector<int>> banks;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwBrainRE1 *hwBrainRE1 = HwBrainRE1::GetInstance(uavoManager);
    if (!hwBrainRE1 || (hwBrainRE1->getMultiPortMode() == 0)) {
        banks.resize(4);
        banks[0] = QVector<int>() << 1 << 2 << 3 << 4; // TIM5
        banks[1] = QVector<int>() << 5; // TIM1
        banks[2] = QVector<int>() << 6; // TIM2
        banks[3] = QVector<int>() << 7 << 8; // TIM8
    } else {
        banks.resize(3);
        banks[0] = QVector<int>() << 1 << 2; // TIM5
        banks[1] = QVector<int>() << 3; // TIM1
        banks[2] = QVector<int>() << 4; // TIM2
    }

    return banks;
}

bool BrainRE1::hasAnnunciator(AnnunciatorType annunc)
{
    switch (annunc) {
    case ANNUNCIATOR_HEARTBEAT:
    case ANNUNCIATOR_ALARM:
    case ANNUNCIATOR_BUZZER:
    case ANNUNCIATOR_RGB:
        return true;
    case ANNUNCIATOR_DAC:
        break;
    }
    return false;
}
