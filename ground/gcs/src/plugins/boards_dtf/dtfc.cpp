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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "dtfc.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>

#include "hwdtfc.h"
#include "dtfcconfiguration.h"

/**
 * @brief DTFc:DTFc
 *  This is the DTFc board definition
 */
DTFc::DTFc(void)
{
    // Initialize our USB Structure definition here:
    USBInfo board;
    board.vendorID = 0x16d0;
    board.productID = 0xd7fc;
    setUSBInfo(board);

    boardType = 0xD7;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(3);
    channelBanks[0] = QVector<int> () << 1 << 2; // TIM4
    channelBanks[1] = QVector<int> () << 3 << 4 << 5 << 6; // TIM2
    channelBanks[2] = QVector<int> () << 7 << 8; // TIM3
}

DTFc::~DTFc()
{

}

QString DTFc::shortName()
{
    return QString("DTFc");
}

QString DTFc::boardDescription()
{
    return QString("DTFc Flight Controller");
}

//! Return which capabilities this board has
bool DTFc::queryCapabilities(BoardCapabilities capability)
{
    switch(capability) {
    case BOARD_CAPABILITIES_GYROS:
        return true;
    case BOARD_CAPABILITIES_ACCELS:
        return true;
    case BOARD_CAPABILITIES_MAGS:
        return false;
    case BOARD_CAPABILITIES_BAROS:
        return true;
    case BOARD_CAPABILITIES_RADIO:
        return false;
    case BOARD_CAPABILITIES_OSD:
        return false;
    default:
        return false;
    }
    return false;
}

/**
 * @brief DTFc::getSupportedProtocols
 *  TODO: this is just a stub, we'll need to extend this a lot with multi protocol support
 * @return
 */
QStringList DTFc::getSupportedProtocols()
{

    return QStringList("uavtalk");
}

QPixmap DTFc::getBoardPicture()
{
    return QPixmap(":/dtf/images/dtfc.png");
}

QString DTFc::getHwUAVO()
{
    return "HwDTFc";
}

//! Determine if this board supports configuring the receiver
bool DTFc::isInputConfigurationSupported(enum InputType type = INPUT_TYPE_ANY)
{
    switch (type) {
    case INPUT_TYPE_PWM:
        return false;
    default:
        return true;
    }
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @return true if successfully configured or false otherwise
 */
bool DTFc::setInputType(enum InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwDTFc *hwDTFc = HwDTFc::GetInstance(uavoManager);
    Q_ASSERT(hwDTFc);
    if (!hwDTFc)
        return false;

    HwDTFc::DataFields settings = hwDTFc->getData();

    switch(type) {
    case INPUT_TYPE_PPM:
        settings.RcvrPort = HwDTFc::RCVRPORT_PPM;
        break;
    case INPUT_TYPE_SBUS:
        settings.RcvrPort = HwDTFc::RCVRPORT_SBUS;
        break;
    case INPUT_TYPE_DSM:
        settings.RcvrPort = HwDTFc::RCVRPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.RcvrPort = HwDTFc::RCVRPORT_HOTTSUMD;
        break;
    default:
        return false;
    }

    hwDTFc->setData(settings);

    return true;
}

/**
 * @brief DTFc::getInputOnPort fetch the currently selected input type
 * @return the selected input type
 */
enum Core::IBoardType::InputType DTFc::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwDTFc *hwDTFc = HwDTFc::GetInstance(uavoManager);
    Q_ASSERT(hwDTFc);
    if (!hwDTFc)
        return INPUT_TYPE_UNKNOWN;

    HwDTFc::DataFields settings = hwDTFc->getData();

    switch(settings.RcvrPort) {
    case HwDTFc::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwDTFc::RCVRPORT_SBUS:
    case HwDTFc::RCVRPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUS;
    case HwDTFc::RCVRPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwDTFc::RCVRPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    }
    switch(settings.Uart1) {
    case HwDTFc::UART1_SBUS:
    case HwDTFc::UART1_SBUSNONINVERTED:
        return INPUT_TYPE_SBUS;
    case HwDTFc::UART1_DSM:
        return INPUT_TYPE_DSM;
    case HwDTFc::UART1_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    }
    switch(settings.Uart2) {
    case HwDTFc::UART2_SBUS:
    case HwDTFc::UART2_SBUSNONINVERTED:
        return INPUT_TYPE_SBUS;
    case HwDTFc::UART2_DSM:
        return INPUT_TYPE_DSM;
    case HwDTFc::UART2_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    }
    
    return INPUT_TYPE_UNKNOWN;
}

int DTFc::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwDTFc *hwDTFc = HwDTFc::GetInstance(uavoManager);
    Q_ASSERT(hwDTFc);
    if (!hwDTFc)
        return 0;

    HwDTFc::DataFields settings = hwDTFc->getData();

    switch(settings.GyroRange) {
    case HwDTFc::GYRORANGE_250:
        return 250;
    case HwDTFc::GYRORANGE_500:
        return 500;
    case HwDTFc::GYRORANGE_1000:
        return 1000;
    case HwDTFc::GYRORANGE_2000:
        return 2000;
    default:
        return 500;
    }
}

QStringList DTFc::getAdcNames()
{
    return QStringList() << "Current" << "Battery";
}

QWidget *DTFc::getBoardConfiguration(QWidget *parent, bool connected)
{
    Q_UNUSED(connected);
    return new DTFcConfiguration(parent);
}
