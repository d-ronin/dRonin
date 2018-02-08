/**
 ******************************************************************************
 * @file       taulink.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_TauLabsPlugin Tau Labs boards support Plugin
 * @{
 * @brief Plugin to support boards by the Tau Labs project
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

#include "taulink.h"

#include "uavobjects/uavobjectmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "board_usb_ids.h"

/**
 * @brief TauLink::TauLink
 *  This is the PipXtreme radio modem definition
 */
TauLink::TauLink(void)
{
    // Common USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_BOOTLOADER, DRONIN_PID_DRONIN_BOOTLOADER, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_DRONIN_FIRMWARE, DRONIN_PID_DRONIN_FIRMWARE, BCD_DEVICE_FIRMWARE));
    // Legacy USB IDs
    addBootloaderUSBInfo(
        USBInfo(DRONIN_VID_OPENPILOT_PIPX, DRONIN_PID_OPENPILOT_PIPX, BCD_DEVICE_BOOTLOADER));
    addFirmwareUSBInfo(
        USBInfo(DRONIN_VID_OPENPILOT_PIPX, DRONIN_PID_OPENPILOT_PIPX, BCD_DEVICE_FIRMWARE));

    boardType = 0x03;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    uavoUtilManager = pm->getObject<UAVObjectUtilManager>();
}

TauLink::~TauLink()
{
}

QString TauLink::shortName()
{
    return QString("PipXtreme");
}

QString TauLink::boardDescription()
{
    return QString("The TauLink radio modem");
}

//! Return which capabilities this board has
bool TauLink::queryCapabilities(BoardCapabilities capability)
{
    switch (capability) {
    case BOARD_CAPABILITIES_RADIO:
        return true;
    default:
        return false;
    }
    return false;
}

QPixmap TauLink::getBoardPicture()
{
    return QPixmap(":/taulabs/images/taulink.png");
}

QString TauLink::getHwUAVO()
{
    return "HwTauLink";
}

//! Get the settings object
HwTauLink *TauLink::getSettings()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();

    HwTauLink *wwTauLink = HwTauLink::GetInstance(uavoManager);
    Q_ASSERT(wwTauLink);

    return wwTauLink;
}
