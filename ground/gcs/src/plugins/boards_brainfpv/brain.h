/**
 ******************************************************************************
 * @file       sparky.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_BrainFPV BrainFPV boards support Plugin
 * @{
 * @brief Plugin to support boards by BrainFPV LLC
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
#ifndef BRAIN_H
#define BRAIN_H

#include <coreplugin/iboardtype.h>

class Brain : public Core::IBoardType
{
public:
    Brain();
    virtual ~Brain();

    QString shortName();
    QString boardDescription();
    int minBootLoaderVersion();
    bool queryCapabilities(BoardCapabilities capability);
    QPixmap getBoardPicture();
    QString getHwUAVO();

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

    int queryMaxGyroRate();
    QWidget * getBoardConfiguration(QWidget *parent, bool connected);
    QStringList getAdcNames();
};


#endif // BRAIN_H
