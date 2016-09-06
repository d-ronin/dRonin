/**
 ******************************************************************************
 * @file       playuavosd.cpp
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

#include "playuavosd.h"

#include "hwsimulation.h"
#include "simulationconfiguration.h"

/**
 * @brief PlayUavOsd:PlayUavOsd
 *  This is the PlayUavOsd board definition
 */
PlayUavOsd::PlayUavOsd(void)
{
    boardType = 0xCB;

    USBInfo board;
    board.vendorID = 0x20A0;
    board.productID = 0x4250;

    setUSBInfo(board);
}

PlayUavOsd::~PlayUavOsd()
{
}

QString PlayUavOsd::shortName()
{
    return QString("PlayUAVOSD");
}

QString PlayUavOsd::boardDescription()
{
    return QString("PlayUAVOSD running dRonin firmware");
}

bool PlayUavOsd::queryCapabilities(BoardCapabilities capability)
{
    switch (capability) {
    case BOARD_CAPABILITIES_OSD:
        return true;
    default:
        break;
    }
    return false;
}

QStringList PlayUavOsd::getSupportedProtocols()
{
    return QStringList("uavtalk");
}

QPixmap PlayUavOsd::getBoardPicture()
{
    return QPixmap(":/images/gcs_logo_256.png");
}

QString PlayUavOsd::getHwUAVO()
{
    return "HwPlayUavOsd";
}

/**
 * @}
 * @}
 */
