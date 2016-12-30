/**
 ******************************************************************************
 * @file       seppuku.cpp
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include <extensionsystem/pluginmanager.h>

#include "seppuku.h"
#include "hwseppuku.h"
#include "seppukuconfiguration.h"

/**
 * @brief Seppuku:Seppuku
 *  This is the Seppuku board definition
 */
Seppuku::Seppuku(void)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    uavoManager = pm->getObject<UAVObjectManager>();

    boardType = 0xA1;

    USBInfo board;
    board.vendorID = 0x20A0;
    board.productID = 0x4250;

    setUSBInfo(board);

    channelBanks.resize(4);
    channelBanks.clear();
    channelBanks << (QVector<int>() << 1 << 2);      // TIM8
    channelBanks << (QVector<int>() << 3);           // TIM14
    channelBanks << (QVector<int>() << 4 << 5 << 6); // TIM3
    channelBanks << (QVector<int>() << 7 << 8);      // TIM5
}

Seppuku::~Seppuku()
{
}

QString Seppuku::shortName()
{
    return QString("Seppuku");
}

QString Seppuku::boardDescription()
{
    return QString("Seppuku running dRonin firmware");
}

bool Seppuku::queryCapabilities(BoardCapabilities capability)
{
    switch (capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_MAGS:
    case BOARD_CAPABILITIES_BAROS:
    case BOARD_CAPABILITIES_OSD:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    default:
        break;
    }
    return false;
}

bool Seppuku::isInputConfigurationSupported(InputType type)
{
    Q_UNUSED(type)
    return false;
}

bool Seppuku::setInputType(InputType type)
{
    UAVObjectField *rcvrPort = uavoManager->getField(getHwUAVO(), "RcvrPort");
    if (!rcvrPort)
        return false;

    switch (type) {
    case INPUT_TYPE_DISABLED:
        rcvrPort->setValue(HwSeppuku::RCVRPORT_DISABLED);
        break;
    case INPUT_TYPE_DSM:
        rcvrPort->setValue(HwSeppuku::RCVRPORT_DSM);
        break;
    case INPUT_TYPE_SBUS:
        rcvrPort->setValue(HwSeppuku::RCVRPORT_SBUS);
        break;
    case INPUT_TYPE_SBUSNONINVERTED:
        rcvrPort->setValue(HwSeppuku::RCVRPORT_SBUSNONINVERTED);
        break;
    case INPUT_TYPE_HOTTSUMD:
        rcvrPort->setValue(HwSeppuku::RCVRPORT_HOTTSUMD);
        break;
    case INPUT_TYPE_HOTTSUMH:
        rcvrPort->setValue(HwSeppuku::RCVRPORT_HOTTSUMH);
        break;
    case INPUT_TYPE_IBUS:
        rcvrPort->setValue(HwSeppuku::RCVRPORT_IBUS);
        break;
    case INPUT_TYPE_PPM:
        rcvrPort->setValue(HwSeppuku::RCVRPORT_PPM);
        break;
    case INPUT_TYPE_SRXL:
        rcvrPort->setValue(HwSeppuku::RCVRPORT_SRXL);
        break;
    case INPUT_TYPE_PWM:
    case INPUT_TYPE_UNKNOWN:
    case INPUT_TYPE_ANY:
        return false;
    }

    return true;
}

enum Core::IBoardType::InputType Seppuku::getInputType()
{
    UAVObjectField *rcvrPort = uavoManager->getField(getHwUAVO(), "RcvrPort");
    if (!rcvrPort)
        return INPUT_TYPE_UNKNOWN;

    switch (static_cast<HwSeppuku::RcvrPortOptions>(rcvrPort->getValue().toInt())) {
    case HwSeppuku::RCVRPORT_DISABLED:
        return INPUT_TYPE_DISABLED;
    case HwSeppuku::RCVRPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwSeppuku::RCVRPORT_SRXL:
        return INPUT_TYPE_SRXL;
    case HwSeppuku::RCVRPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    case HwSeppuku::RCVRPORT_HOTTSUMH:
        return INPUT_TYPE_HOTTSUMH;
    case HwSeppuku::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwSeppuku::RCVRPORT_SBUS:
        return INPUT_TYPE_SBUS;
    case HwSeppuku::RCVRPORT_SBUSNONINVERTED:
        return INPUT_TYPE_SBUSNONINVERTED;
    case HwSeppuku::RCVRPORT_IBUS:
        return INPUT_TYPE_IBUS;
    }

    return INPUT_TYPE_UNKNOWN;
}

int Seppuku::queryMaxGyroRate()
{
    return 2000;
}

QStringList Seppuku::getAdcNames()
{
    return QStringList() << "V" << "A" << "RSSI" << "Unmarked";
}

QPixmap Seppuku::getBoardPicture()
{
    return QPixmap(":/dronin/images/seppuku.png");
}

QString Seppuku::getHwUAVO()
{
    return "HwSeppuku";
}

QString Seppuku::getConnectionDiagram()
{
    return ":/dronin/images/seppuku-connection.svg";
}

QWidget *Seppuku::getBoardConfiguration(QWidget *parent, bool connected)
{
    Q_UNUSED(connected)

    return new SeppukuConfiguration(parent);
}

/**
 * @}
 * @}
 */
