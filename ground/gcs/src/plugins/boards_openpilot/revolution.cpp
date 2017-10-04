/**
 ******************************************************************************
 *
 * @file       revolution.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_OpenPilotPlugin OpenPilot boards support Plugin
 * @{
 * @brief Plugin to support boards by the OP project
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

#include "revolution.h"
#include "rfm22bstatus.h"

#include "uavobjects/uavobjectmanager.h"
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "hwrevolution.h"

/**
 * @brief Revolution::Revolution
 *  This is the Revolution board definition
 */
Revolution::Revolution(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));
    // Legacy USB IDs
    addBootloaderUSBInfo(USBInfo(DRONIN_VID_OPENPILOT_REVOLUTION, DRONIN_PID_OPENPILOT_REVOLUTION,
                                 BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(USBInfo(DRONIN_VID_OPENPILOT_REVOLUTION, DRONIN_PID_OPENPILOT_REVOLUTION,
                               BCD_DEVICE_FIRMWARE));

    boardType = 0x09;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(6);
    channelBanks[0] = QVector<int>() << 1 << 2; // Tim3
    channelBanks[1] = QVector<int>() << 3; // Tim9
    channelBanks[2] = QVector<int>() << 4; // Tim2
    channelBanks[3] = QVector<int>() << 5 << 6; // Tim5
    channelBanks[4] = QVector<int>() << 7 << 12; // Tim12
    channelBanks[5] = QVector<int>() << 8 << 9 << 10 << 11; // Tim8

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    uavoUtilManager = pm->getObject<UAVObjectUtilManager>();
}

Revolution::~Revolution()
{
}

int Revolution::minBootLoaderVersion()
{
    return 0x84;
}

QString Revolution::shortName()
{
    return QString("Revolution");
}

QString Revolution::boardDescription()
{
    return QString("The OpenPilot project Revolution board");
}

//! Return which capabilities this board has
bool Revolution::queryCapabilities(BoardCapabilities capability)
{
    switch (capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_MAGS:
    case BOARD_CAPABILITIES_BAROS:
    case BOARD_CAPABILITIES_RADIO:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    default:
        break;
    }

    return false;
}

QPixmap Revolution::getBoardPicture()
{
    return QPixmap(":/openpilot/images/revolution.png");
}

QString Revolution::getHwUAVO()
{
    return "HwRevolution";
}

//! Get the settings object
HwRevolution *Revolution::getSettings()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();

    HwRevolution *hwRevolution = HwRevolution::GetInstance(uavoManager);
    Q_ASSERT(hwRevolution);

    return hwRevolution;
}
//! Determine if this board supports configuring the receiver
bool Revolution::isInputConfigurationSupported(Core::IBoardType::InputType type)
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
bool Revolution::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwRevolution *hwRevolution = HwRevolution::GetInstance(uavoManager);
    Q_ASSERT(hwRevolution);
    if (!hwRevolution)
        return false;

    HwRevolution::DataFields settings = hwRevolution->getData();

    switch (type) {
    case INPUT_TYPE_PPM:
        settings.RxPort = HwRevolution::RXPORT_PPM;
        break;
    case INPUT_TYPE_PWM:
        settings.RxPort = HwRevolution::RXPORT_PWM;
        break;
    case INPUT_TYPE_SBUS:
        settings.MainPort = HwRevolution::MAINPORT_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.MainPort = HwRevolution::MAINPORT_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_DSM:
        settings.FlexiPort = HwRevolution::FLEXIPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.FlexiPort = HwRevolution::FLEXIPORT_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.FlexiPort = HwRevolution::FLEXIPORT_HOTTSUMH;
        break;
    case INPUT_TYPE_IBUS:
        settings.FlexiPort = HwRevolution::FLEXIPORT_IBUS;
        break;
    case INPUT_TYPE_SRXL:
        settings.FlexiPort = HwRevolution::FLEXIPORT_SRXL;
        break;
    case INPUT_TYPE_TBSCROSSFIRE:
        settings.FlexiPort = HwRevolution::FLEXIPORT_TBSCROSSFIRE;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwRevolution->setData(settings);

    return true;
}

/**
 * @brief Revolution::getInputType fetch the currently selected input type
 * @return the selected input type
 */
Core::IBoardType::InputType Revolution::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwRevolution *hwRevolution = HwRevolution::GetInstance(uavoManager);
    Q_ASSERT(hwRevolution);
    if (!hwRevolution)
        return INPUT_TYPE_UNKNOWN;

    HwRevolution::DataFields settings = hwRevolution->getData();

    switch (settings.MainPort) {
    case HwRevolution::MAINPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwRevolution::MAINPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwRevolution::MAINPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwRevolution::MAINPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwRevolution::MAINPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwRevolution::MAINPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwRevolution::MAINPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwRevolution::MAINPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.FlexiPort) {
    case HwRevolution::FLEXIPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwRevolution::FLEXIPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwRevolution::FLEXIPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwRevolution::FLEXIPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwRevolution::FLEXIPORT_IBUS:
        return INPUT_TYPE_IBUS;
    case HwRevolution::FLEXIPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwRevolution::FLEXIPORT_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }

    switch (settings.RxPort) {
    case HwRevolution::RXPORT_PPM:
    case HwRevolution::RXPORT_PPMPWM:
    case HwRevolution::RXPORT_PPMOUTPUTS:
    case HwRevolution::RXPORT_PPMUART:
    case HwRevolution::RXPORT_PPMFRSKY:
        return INPUT_TYPE_PPM;
    case HwRevolution::RXPORT_PWM:
        return INPUT_TYPE_PWM;
    case HwRevolution::RXPORT_UART:
        switch (settings.RxPortUsart) {
        case HwRevolution::RXPORTUSART_DSM:
            return INPUT_TYPE_DSM;
        case HwRevolution::RXPORTUSART_HOTTSUMD:
            return INPUT_TYPE_HOTTSUMD;
        case HwRevolution::RXPORTUSART_HOTTSUMH:
            return INPUT_TYPE_HOTTSUMH;
        case HwRevolution::RXPORTUSART_SBUSNONINVERTED:
            return INPUT_TYPE_SBUSNONINVERTED;
        case HwRevolution::RXPORTUSART_IBUS:
            return INPUT_TYPE_IBUS;
        case HwRevolution::RXPORTUSART_SRXL:
            return INPUT_TYPE_SRXL;
        case HwRevolution::RXPORTUSART_TBSCROSSFIRE:
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

int Revolution::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwRevolution *hwRevolution = HwRevolution::GetInstance(uavoManager);
    Q_ASSERT(hwRevolution);
    if (!hwRevolution)
        return 0;

    HwRevolution::DataFields settings = hwRevolution->getData();

    switch (settings.GyroRange) {
    case HwRevolution::GYRORANGE_250:
        return 250;
    case HwRevolution::GYRORANGE_500:
        return 500;
    case HwRevolution::GYRORANGE_1000:
        return 1000;
    case HwRevolution::GYRORANGE_2000:
        return 2000;
    default:
        break;
    }

    return 2000;
}

/**
 * Get the RFM22b device ID this modem
 * @return RFM22B device ID or 0 if not supported
 */
quint32 Revolution::getRfmID()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();

    // Flight controllers are instance 1
    RFM22BStatus *rfm22bStatus = RFM22BStatus::GetInstance(uavoManager, 1);
    Q_ASSERT(rfm22bStatus);
    RFM22BStatus::DataFields rfm22b = rfm22bStatus->getData();

    return rfm22b.DeviceID;
}

/**
 * Set the coordinator ID. If set to zero this device will
 * be a coordinator.
 * @return true if successful or false if not
 */
bool Revolution::bindRadio(quint32 id, quint32 baud_rate, float rf_power,
                           Core::IBoardType::LinkMode linkMode, quint8 min, quint8 max)
{
    HwRevolution::DataFields settings = getSettings()->getData();

    settings.CoordID = id;

    switch (baud_rate) {
    case 9600:
        settings.MaxRfSpeed = HwRevolution::MAXRFSPEED_9600;
        break;
    case 19200:
        settings.MaxRfSpeed = HwRevolution::MAXRFSPEED_19200;
        break;
    case 32000:
        settings.MaxRfSpeed = HwRevolution::MAXRFSPEED_32000;
        break;
    case 64000:
        settings.MaxRfSpeed = HwRevolution::MAXRFSPEED_64000;
        break;
    case 100000:
        settings.MaxRfSpeed = HwRevolution::MAXRFSPEED_100000;
        break;
    case 192000:
        settings.MaxRfSpeed = HwRevolution::MAXRFSPEED_192000;
        break;
    }

    // Round to an integer to use a switch statement
    quint32 rf_power_100 = (rf_power * 100) + 0.5;
    switch (rf_power_100) {
    case 0:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_0;
        break;
    case 125:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_125;
        break;
    case 160:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_16;
        break;
    case 316:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_316;
        break;
    case 630:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_63;
        break;
    case 1260:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_126;
        break;
    case 2500:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_25;
        break;
    case 5000:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_50;
        break;
    case 10000:
        settings.MaxRfPower = HwRevolution::MAXRFPOWER_100;
        break;
    }

    switch (linkMode) {
    case Core::IBoardType::LINK_TELEM:
        settings.Radio = HwRevolution::RADIO_TELEM;
        break;
    case Core::IBoardType::LINK_TELEM_PPM:
        settings.Radio = HwRevolution::RADIO_TELEMPPM;
        break;
    case Core::IBoardType::LINK_PPM:
        settings.Radio = HwRevolution::RADIO_PPM;
        break;
    }

    settings.MinChannel = min;
    settings.MaxChannel = max;

    getSettings()->setData(settings);
    uavoUtilManager->saveObjectToFlash(getSettings());

    return true;
}

QStringList Revolution::getAdcNames()
{
    return QStringList() << "Pwr Sen Pin 3"
                         << "Pwr Sen Pin 4";
}

bool Revolution::hasAnnunciator(AnnunciatorType annunc)
{
    switch (annunc) {
    case ANNUNCIATOR_HEARTBEAT:
    case ANNUNCIATOR_RGB:
        return true;
    case ANNUNCIATOR_BUZZER:
        /* TODO: would be nice to have a distinct "revision" for this */
    case ANNUNCIATOR_ALARM:
        if (uavoUtilManager->getBoardRevision() == 3)
            return true;
        else
            break;
    case ANNUNCIATOR_DAC:
        break;
    }
    return false;
}
