/**
 ******************************************************************************
 *
 * @file       colibri.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_TBS TBS boards support Plugin
 * @{
 * @brief Plugin to support boards by Team Black Sheep
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
#ifndef COLIBRI_H
#define COLIBRI_H

#include <coreplugin/iboardtype.h>

class IBoardType;

class Colibri : public Core::IBoardType
{
public:
    Colibri();
    virtual ~Colibri();

    virtual QString shortName();
    virtual QString boardDescription();
    virtual bool queryCapabilities(BoardCapabilities capability);
    virtual QStringList getSupportedProtocols();
    virtual QPixmap getBoardPicture();
    virtual QString getHwUAVO();
    virtual int queryMaxGyroRate();
    virtual QWidget *getBoardConfiguration(QWidget *parent, bool connected);

    //! Tell the wizard this board knows how to configure inputs
    bool isInputConfigurationSupported(enum InputType type = INPUT_TYPE_ANY) { Q_UNUSED(type); return true; }

    /**
     * Configure the board to use an receiver input type on a port number
     * @param type the type of receiver to use
     */
    virtual bool setInputType(enum InputType type);

    /**
     * @brief getInputType get the current input type
     * @return the currently selected input type
     */
    virtual enum InputType getInputType();
    virtual QStringList getAdcNames();
};


#endif // COLIBRI_H
