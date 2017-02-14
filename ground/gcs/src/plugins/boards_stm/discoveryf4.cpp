/**
 ******************************************************************************
 *
 * @file       discoveryf4.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_Stm STM boards support Plugin
 * @{
 * @brief Plugin to support boards by STM
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

#include "discoveryf4.h"
#include "board_usb_ids.h"

/**
 * @brief DiscoveryF4::DiscoveryF4
 *  This is the DiscoveryF4 board definition
 */
DiscoveryF4::DiscoveryF4(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));
    // Legacy USB IDs
    addBootloaderUSBInfo(USBInfo(DRONIN_VID_OPENPILOT_SPARE, DRONIN_PID_OPENPILOT_SPARE, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(USBInfo(DRONIN_VID_OPENPILOT_SPARE, DRONIN_PID_OPENPILOT_SPARE, BCD_DEVICE_FIRMWARE));

    boardType = 0x85;
}

DiscoveryF4::~DiscoveryF4()
{

}

QString DiscoveryF4::shortName()
{
    return QString("discoveryf4");
}

QString DiscoveryF4::boardDescription()
{
    return QString("DiscoveryF4");
}

//! Return which capabilities this board has
bool DiscoveryF4::queryCapabilities(BoardCapabilities capability)
{
    Q_UNUSED(capability);
    return false;
}

QPixmap DiscoveryF4::getBoardPicture()
{
    return QPixmap();
}

QString DiscoveryF4::getHwUAVO()
{
    return "HwDiscoveryF4";
}
