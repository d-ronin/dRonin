/**
 ******************************************************************************
 * @file       sprf3e.cpp
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016, 2017
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_dRonin dRonin board support plugin
 * @{
 * @brief Supports dRonin board configuration
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

#include <extensionsystem/pluginmanager.h>

#include "sprf3e.h"
#include "hwsprf3e.h"
#include "sprf3econfiguration.h"
#include "board_usb_ids.h"


#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"


/**
 * @brief Sprf3e:Sprf3e
 *  This is the Sprf3e board definition
 */
Sprf3e::Sprf3e(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));

    boardType = 0xCF;

    channelBanks.resize(3);
    channelBanks.clear();
    channelBanks << (QVector<int>() << 1 << 2);           // TIM2
    channelBanks << (QVector<int>() << 3 << 4);           // TIM15
    channelBanks << (QVector<int>() << 5 << 6 << 7 << 8); // TIM3
}

Sprf3e::~Sprf3e()
{
}

QString Sprf3e::shortName()
{
    return QString("Sprf3e");
}

QString Sprf3e::boardDescription()
{
    return QString(tr("SP Racing F3 E / FrSky XSRF3E running dRonin firmware"));
}

bool Sprf3e::queryCapabilities(BoardCapabilities capability)
{
    switch (capability) {
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

bool Sprf3e::isInputConfigurationSupported(Core::IBoardType::InputType type)
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

bool Sprf3e::setInputType(Core::IBoardType::InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwSprf3e *hwSprf3e = HwSprf3e::GetInstance(uavoManager);
    Q_ASSERT(hwSprf3e);
    if (!hwSprf3e)
        return false;

    HwSprf3e::DataFields settings = hwSprf3e->getData();

    switch(type) {
    case INPUT_TYPE_DSM:
        settings.Uart1 = HwSprf3e::UART2_DSM;
        break;    
    case INPUT_TYPE_SRXL:
        settings.Uart1 = HwSprf3e::UART2_SRXL;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.Uart2 = HwSprf3e::UART2_HOTTSUMD;
        break;
    case INPUT_TYPE_HOTTSUMH:
        settings.Uart1 = HwSprf3e::UART2_HOTTSUMD;
        break;
    case INPUT_TYPE_PPM:
        settings.Uart2 = HwSprf3e::UART2_PPM;
        break;
    case INPUT_TYPE_SBUS:
        settings.Uart2 = HwSprf3e::UART2_SBUS;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.Uart1 = HwSprf3e::UART2_SBUSNONINVERTED;
        break;
    case INPUT_TYPE_IBUS:
        settings.Uart1 = HwSprf3e::UART2_IBUS;
        break;
    case INPUT_TYPE_TBSCROSSFIRE:
        settings.Uart1 = HwSprf3e::UART2_TBSCROSSFIRE;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwSprf3e->setData(settings);

    return true;
}

Core::IBoardType::InputType Sprf3e::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwSprf3e *hwSprf3e = HwSprf3e::GetInstance(uavoManager);
    Q_ASSERT(hwSprf3e);
    if (!hwSprf3e)
        return INPUT_TYPE_UNKNOWN;

    HwSprf3e::DataFields settings = hwSprf3e->getData();

    switch(settings.Uart1) {
    case HwSprf3e::UART1_DSM:
        return INPUT_TYPE_DSM;
    case HwSprf3e::UART1_SRXL:
        return INPUT_TYPE_SRXL;
    case HwSprf3e::UART1_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwSprf3e::UART1_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwSprf3e::UART1_SBUS:
        return INPUT_TYPE_SBUS;
    case HwSprf3e::UART1_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwSprf3e::UART1_IBUS:
        return INPUT_TYPE_IBUS;
    case HwSprf3e::UART1_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }
    
    switch(settings.Uart2) {
    case HwSprf3e::UART2_DSM:
        return INPUT_TYPE_DSM;
    case HwSprf3e::UART2_SRXL:
        return INPUT_TYPE_SRXL;
    case HwSprf3e::UART2_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwSprf3e::UART2_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwSprf3e::UART2_SBUS:
        return INPUT_TYPE_SBUS;
    case HwSprf3e::UART2_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwSprf3e::UART2_IBUS:
        return INPUT_TYPE_IBUS;
    case HwSprf3e::UART2_PPM:
        return INPUT_TYPE_PPM;
    case HwSprf3e::UART2_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }
    
    switch(settings.Uart3) {
    case HwSprf3e::UART3_DSM:
        return INPUT_TYPE_DSM;
    case HwSprf3e::UART3_SRXL:
        return INPUT_TYPE_SRXL;
    case HwSprf3e::UART3_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwSprf3e::UART3_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwSprf3e::UART3_SBUS:
        return INPUT_TYPE_SBUS;
    case HwSprf3e::UART3_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwSprf3e::UART3_IBUS:
        return INPUT_TYPE_IBUS;
    case HwSprf3e::UART3_TBSCROSSFIRE:
        return INPUT_TYPE_TBSCROSSFIRE;
    default:
        break;
    }
    
    return INPUT_TYPE_PPM;
}

int Sprf3e::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwSprf3e *hwSprf3e = HwSprf3e::GetInstance(uavoManager);
    Q_ASSERT(hwSprf3e);
    if (!hwSprf3e)
        return 0;

    HwSprf3e::DataFields settings = hwSprf3e->getData();

    switch(settings.GyroRange) {
    case HwSprf3e::GYRORANGE_250:
        return 250;
    case HwSprf3e::GYRORANGE_500:
        return 500;
    case HwSprf3e::GYRORANGE_1000:
        return 1000;
    case HwSprf3e::GYRORANGE_2000:
        return 2000;
    default:
        break;
    }
    
    return 2000;
}

QStringList Sprf3e::getAdcNames()
{
    return QStringList() << "V" << "A" << "RSSI";
}

QPixmap Sprf3e::getBoardPicture()
{
    return QPixmap(":/dronin/images/sprf3e.png");
}

QString Sprf3e::getHwUAVO()
{
    return "HwSprf3e";
}

QString Sprf3e::getConnectionDiagram()
{
    return ":/dronin/images/sprf3e-connection.svg";
}

QWidget *Sprf3e::getBoardConfiguration(QWidget *parent, bool connected)
{
    Q_UNUSED(connected)

    return new Sprf3eConfiguration(parent);
}

/**
 * @}
 * @}
 */
