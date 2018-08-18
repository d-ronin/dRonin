/**
 ******************************************************************************
 * @file       f3fc.cpp
 * @author     Dronin, http://dronin.org, Copyright (C) 2016
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_RcExplorerPlugin RcExplorer boards support Plugin
 * @{
 * @brief Plugin to support boards by the RcExplorer
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

#include "f3fc.h"

#include "uavobjects/uavobjectmanager.h"
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

#include "hwf3fc.h"

/**
 * @brief F3Fc::F3Fc
 *  This is the F3Fc board definition
 */
F3Fc::F3Fc(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));

	boardType = 0x96;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(4);
    channelBanks[0] = QVector<int> () << 1;            // TIM1
    channelBanks[1] = QVector<int> () << 2;            // TIM17
    channelBanks[2] = QVector<int> () << 3 << 4 << 5;  // TIM3
	channelBanks[3] = QVector<int> () << 6;            // TIM2
}

F3Fc::~F3Fc()
{

}

QString F3Fc::shortName()
{
    return QString("F3FC");
}

QString F3Fc::boardDescription()
{
    return QString("The F3FC board");
}

//! Return which capabilities this board has
bool F3Fc::queryCapabilities(BoardCapabilities capability)
{
    switch(capability) {
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

/**
 * @brief F3Fc::getSupportedProtocols
 *  TODO: this is just a stub, we'll need to extend this a lot with multi protocol support
 * @return
 */
QStringList F3Fc::getSupportedProtocols()
{

    return QStringList("uavtalk");
}

QPixmap F3Fc::getBoardPicture()
{
    return QPixmap(":/rcexplorer/images/f3fc.png");
}

QString F3Fc::getHwUAVO()
{
    return "HwF3Fc";
}

int F3Fc::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwF3Fc *hwf3fc = HwF3Fc::GetInstance(uavoManager);
    Q_ASSERT(hwf3fc);
    if (!hwf3fc)
        return 0;

    HwF3Fc::DataFields settings = hwf3fc->getData();

    switch(settings.GyroRange) {
    case HwF3Fc::GYRORANGE_250:
        return 250;
    case HwF3Fc::GYRORANGE_500:
        return 500;
    case HwF3Fc::GYRORANGE_1000:
        return 1000;
    case HwF3Fc::GYRORANGE_2000:
        return 2000;
    default:
        return 500;
    }
}

QStringList F3Fc::getAdcNames()
{
    return QStringList() << "Current" << "Battery" << "RSSI" << "Fdbk2";
}
