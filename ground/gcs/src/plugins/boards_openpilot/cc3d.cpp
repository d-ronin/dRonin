/**
 ******************************************************************************
 *
 * @file       cc3d.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "cc3d.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>
#include "config_cc_hw_widget.h"
#include "hwcoptercontrol.h"

/**
 * @brief CC3D::CC3D
 *  This is the CopterControl (3D) board definition
 */
CC3D::CC3D(void)
{
    // Initialize our USB Structure definition here:
    USBInfo board;
    board.vendorID = 0x20A0;
    board.productID = 0x415b;

    setUSBInfo(board);

    boardType = 0x04;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(6);
    channelBanks[0] = QVector<int> () << 1 << 2 << 3;
    channelBanks[1] = QVector<int> () << 4;
    channelBanks[2] = QVector<int> () << 5 << 7 << 8;
    channelBanks[3] = QVector<int> () << 6 << 9 << 10;
}

CC3D::~CC3D()
{

}

//! Return which capabilities this board has
bool CC3D::queryCapabilities(BoardCapabilities capability)
{
    switch(capability) {
    case BOARD_CAPABILITIES_GYROS:
        return true;
    case BOARD_CAPABILITIES_ACCELS:
        return true;
    case BOARD_CAPABILITIES_MAGS:
        return false;
    case BOARD_CAPABILITIES_BAROS:
        return false;
    case BOARD_CAPABILITIES_RADIO:
        return false;
    case BOARD_CAPABILITIES_OSD:
        return false;
    }
    return false;
}

QString CC3D::shortName()
{
    return QString("CC3D");
}

QString CC3D::boardDescription()
{
    return QString("The OpenPilot project CopterControl 3D board");
}

/**
 * @brief CC3D::getSupportedProtocols
 *  TODO: this is just a stub, we'll need to extend this a lot with multi protocol support
 * @return
 */
QStringList CC3D::getSupportedProtocols()
{

    return QStringList("uavtalk");
}

QPixmap CC3D::getBoardPicture()
{
    return QPixmap(":/openpilot/images/cc3d.png");
}

QString CC3D::getHwUAVO()
{
    return "HwCopterControl";
}


//! Determine if this board supports configuring the receiver
bool CC3D::isInputConfigurationSupported()
{
    return true;
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @return true if successfully configured or false otherwise
 */
bool CC3D::setInputType(enum InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwCopterControl *hwCopterControl = HwCopterControl::GetInstance(uavoManager);
    Q_ASSERT(hwCopterControl);
    if (!hwCopterControl)
        return false;

    HwCopterControl::DataFields settings = hwCopterControl->getData();

    switch(type) {
    case INPUT_TYPE_PWM:
        settings.RcvrPort = HwCopterControl::RCVRPORT_PWM;
        break;
    case INPUT_TYPE_PPM:
	/* Break from the past; for new builds pick a default that will
	 * work with OneShot/SyncPWM. */
        settings.RcvrPort = HwCopterControl::RCVRPORT_PPMONPIN8;
        break;
    case INPUT_TYPE_SBUS:
        settings.MainPort = HwCopterControl::MAINPORT_SBUS;
        settings.FlexiPort = HwCopterControl::FLEXIPORT_TELEMETRY;
        break;
    case INPUT_TYPE_DSM:
        settings.FlexiPort = HwCopterControl::FLEXIPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.FlexiPort = HwCopterControl::FLEXIPORT_HOTTSUMD;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwCopterControl->setData(settings);

    return true;
}

/**
 * @brief CC3D::getInputType fetch the currently selected input type
 * @return the selected input type
 */
enum Core::IBoardType::InputType CC3D::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwCopterControl *hwCopterControl = HwCopterControl::GetInstance(uavoManager);
    Q_ASSERT(hwCopterControl);
    if (!hwCopterControl)
        return INPUT_TYPE_UNKNOWN;

    HwCopterControl::DataFields settings = hwCopterControl->getData();

    switch(settings.FlexiPort) {
    case HwCopterControl::FLEXIPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwCopterControl::FLEXIPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    default:
        break;
    }

    switch(settings.MainPort) {
    case HwCopterControl::MAINPORT_SBUS:
        return INPUT_TYPE_SBUS;
    default:
        break;
    }

    switch(settings.RcvrPort) {
    case HwCopterControl::RCVRPORT_PPMONPIN8:
    case HwCopterControl::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwCopterControl::RCVRPORT_PWM:
        return INPUT_TYPE_PWM;
    default:
        break;
    }

    return INPUT_TYPE_UNKNOWN;
}

int CC3D::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwCopterControl *hwCopterControl = HwCopterControl::GetInstance(uavoManager);
    UAVObjectUtilManager* utilMngr = pm->getObject<UAVObjectUtilManager>();
    Q_ASSERT(hwCopterControl);
    Q_ASSERT(utilMngr);
    if (!hwCopterControl)
        return 0;
    HwCopterControl::DataFields settings = hwCopterControl->getData();

    switch(settings.GyroRange) {
    case HwCopterControl::GYRORANGE_250:
        return 250;
    case HwCopterControl::GYRORANGE_500:
        return 500;
    case HwCopterControl::GYRORANGE_1000:
        return 1000;
    case HwCopterControl::GYRORANGE_2000:
        return 2000;
    default:
        return 500;
    }
}

/**
 * @brief CC3D::getBoardConfiguration create the custom configuration
 * dialog for CC3D.
 * @param parent
 * @param connected
 * @return
 */
QWidget * CC3D::getBoardConfiguration(QWidget *parent, bool connected)
{
    Q_UNUSED(connected);
    return new ConfigCCHWWidget(parent);
}
