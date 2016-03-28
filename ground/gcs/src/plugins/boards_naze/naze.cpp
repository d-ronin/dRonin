/**
 ******************************************************************************
 *
 * @file       naze.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_Naze Naze boards support Plugin
 * @{
 * @brief Plugin to support Naze boards
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

#include "naze.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>

#include "hwnaze.h"

/**
 * @brief Naze:Naze
 *  This is the Naze board definition
 */
Naze::Naze(void)
{
    boardType = 0xA0;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(4);
    channelBanks[0] = QVector<int> () << 1 << 2; // T1
    channelBanks[1] = QVector<int> () << 3 << 4 << 5 << 6; // T4
    channelBanks[2] = QVector<int> () << 7 << 8 << 9 << 10; // T2
    channelBanks[3] = QVector<int> () << 11 << 12 << 13 << 14; // T3
}

Naze::~Naze()
{

}

QString Naze::shortName()
{
    return QString("Naze");
}

QString Naze::boardDescription()
{
    return QString("Open Naze hardware by ReadError, based on Naze32 by timecop.");
}

//! Return which capabilities this board has
bool Naze::queryCapabilities(BoardCapabilities capability)
{
    switch(capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
        return true;
    default:
        return false;
    }
    return false;
}


/**
 * @brief Naze::getSupportedProtocols
 *  TODO: this is just a stub, we'll need to extend this a lot with multi protocol support
 * @return
 */
QStringList Naze::getSupportedProtocols()
{

    return QStringList("uavtalk");
}

QPixmap Naze::getBoardPicture()
{
    return QPixmap(":/naze/images/nazev6.png");
}

QString Naze::getHwUAVO()
{
    return "HwNaze";
}

int Naze::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwNaze *hwNaze = HwNaze::GetInstance(uavoManager);
    Q_ASSERT(hwNaze);
    if (!hwNaze)
        return 0;

    HwNaze::DataFields settings = hwNaze->getData();

    switch(settings.GyroRange) {
    case HwNaze::GYRORANGE_250:
        return 250;
    case HwNaze::GYRORANGE_500:
        return 500;
    case HwNaze::GYRORANGE_1000:
        return 1000;
    case HwNaze::GYRORANGE_2000:
        return 2000;
    default:
        return 500;
    }
}

//! Determine if this board supports configuring the receiver
bool Naze::isInputConfigurationSupported(enum InputType type = INPUT_TYPE_ANY)
{
    // doesn't work for now since the board can't reconnect  automatically after reboot
    Q_UNUSED(type);
    return false;
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @return true if successfully configured or false otherwise
 */
bool Naze::setInputType(enum InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwNaze *hwNaze = HwNaze::GetInstance(uavoManager);
    Q_ASSERT(hwNaze);
    if (!hwNaze)
        return false;

    HwNaze::DataFields settings = hwNaze->getData();

    switch(type) {
    case INPUT_TYPE_PPM:
        settings.RcvrPort = HwNaze::RCVRPORT_PPM;
        break;
    case INPUT_TYPE_DSM:
        settings.RcvrPort = HwNaze::RCVRPORT_SERIAL;
        settings.RcvrSerial = HwNaze::RCVRSERIAL_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.RcvrPort = HwNaze::RCVRPORT_SERIAL;
        settings.RcvrSerial = HwNaze::RCVRSERIAL_HOTTSUMD;
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        settings.RcvrPort = HwNaze::RCVRPORT_SERIAL;
        settings.RcvrSerial = HwNaze::RCVRSERIAL_SBUSNONINVERTED;
        break;

    default:
        return false;
    }

    // Apply these changes
    hwNaze->setData(settings);

    return true;
}

/**
 * @brief Naze::getInputType fetch the currently selected input type
 * @return the selected input type
 */
enum Core::IBoardType::InputType Naze::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwNaze *hwNaze = HwNaze::GetInstance(uavoManager);
    Q_ASSERT(hwNaze);
    if (!hwNaze)
        return INPUT_TYPE_UNKNOWN;

    HwNaze::DataFields settings = hwNaze->getData();

    switch(settings.RcvrPort) {
    case HwNaze::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwNaze::RCVRPORT_SERIAL:
        switch(settings.RcvrSerial) {
        case HwNaze::RCVRSERIAL_DSM:
            return INPUT_TYPE_DSM;
        case HwNaze::RCVRSERIAL_HOTTSUMD:
            return INPUT_TYPE_HOTTSUMD;
        case HwNaze::RCVRSERIAL_SBUSNONINVERTED:
            return INPUT_TYPE_SBUSNONINVERTED;
        default:
            // can still use PPM
            return INPUT_TYPE_PPM;
        }
    default:
        return INPUT_TYPE_UNKNOWN;
    }
}

QStringList Naze::getAdcNames()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwNaze *hwNaze = HwNaze::GetInstance(uavoManager);
    Q_ASSERT(hwNaze);
    if (!hwNaze)
        return QStringList();

    QStringList names = QStringList() << "Batt" << "ADC/ADC5 Pad";
    HwNaze::DataFields settings = hwNaze->getData();
    if (settings.RcvrPort == HwNaze::RCVRPORT_PPM || settings.RcvrPort == HwNaze::RCVRPORT_PPMSERIAL || settings.RcvrPort == HwNaze::RCVRPORT_SERIAL)
        names << "RC In 2" << "RC In 8";
    else
        names << "Disabled" << "Disabled";

    return names;
}
