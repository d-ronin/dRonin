/**
 ******************************************************************************
 *
 * @file       lux.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_Brotronics Brotronics boards support Plugin
 * @{
 * @brief Plugin to support Brotronics boards
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
#ifndef LUX_H
#define LUX_H

#include <coreplugin/iboardtype.h>

class Lux : public Core::IBoardType
{
public:
    Lux();
    virtual ~Lux();

    QString shortName();
    QString boardDescription();
    bool queryCapabilities(BoardCapabilities capability);
    QPixmap getBoardPicture();
    QString getHwUAVO();
    int queryMaxGyroRate();

    //! Determine if this board supports configuring the receiver
    bool isInputConfigurationSupported(InputType type);

    /**
    * Configure the board to use an receiver input type on a port number
    * @param type the type of receiver to use
    */
    bool setInputType(InputType type);

    /**
    * @brief getInputType get the current input type
    * @return the currently selected input type
    */
    InputType getInputType();

    /**
    * @brief getConnectionDiagram get the connection diagram for this board
    * @return a string with the name of the resource for this board diagram
    */
    QString getConnectionDiagram() { return ":/brotronics/images/lux-connection-diagram.svg"; }

    QStringList getAdcNames();

    /**
     * @brief getBoardConfiguration
     * @param parent Parent object
     * @param connected Unused
     * @return Configuration widget handle or NULL on failure
     */
    QWidget *getBoardConfiguration(QWidget *parent, bool connected);

};


#endif // LUX_H
