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

int Quanton::minBootLoaderVersion() {
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
    default:
        return false;
    }
    return false;
}

QPixmap Quanton::getBoardPicture()
{
    return QPixmap(":/quantec/images/quanton.png");
}

QString Quanton::getHwUAVO()
{
    return "HwQuanton";
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
    default:
        return 500;
    }
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
