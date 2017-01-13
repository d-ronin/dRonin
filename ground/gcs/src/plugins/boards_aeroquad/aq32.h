/**
 ******************************************************************************
 * @file       aq32.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_AeroQuadPlugin AeroQuad boards support Plugin
 * @{
 * @brief Plugin to support AeroQuad boards
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
 
#ifndef AQ32_H
#define AQ32_H

#include <coreplugin/iboardtype.h>

class AQ32 : public Core::IBoardType
{
public:
    AQ32();
    virtual ~AQ32();

    QString shortName();
    QString boardDescription();
    bool queryCapabilities(BoardCapabilities capability);
    QPixmap getBoardPicture();
    QString getHwUAVO();

    //! Determine if this board supports configuring the receiver
    bool isInputConfigurationSupported(Core::IBoardType::InputType type);

    /**
     * Configure the board to use an receiver input type on a port number
     * @param type the type of receiver to use
     * @param port_num which input port to configure (board specific numbering)
     */
    bool setInputType(Core::IBoardType::InputType type);

    /**
     * @brief getInputOnPort get the current input type
     * @param port_num which input port to query (board specific numbering)
     * @return the currently selected input type
     */
    Core::IBoardType::InputType getInputType();

    int queryMaxGyroRate();
    QStringList getAdcNames();

};


#endif // AQ32_H
