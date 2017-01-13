/**
 ******************************************************************************
 *
 * @file       naze.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
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
#ifndef NAZE_H
#define NAZE_H

#include <coreplugin/iboardtype.h>

class Naze : public Core::IBoardType
{
public:
    Naze();
    virtual ~Naze();

    QString shortName();
    QString boardDescription();
    bool queryCapabilities(BoardCapabilities capability);
    QPixmap getBoardPicture();
    QString getHwUAVO();
    int queryMaxGyroRate();
    bool isInputConfigurationSupported(InputType type);
    bool setInputType(InputType type);
    InputType getInputType();
    bool isUSBSupported() { return false; }
    QStringList getAdcNames();
};


#endif // NAZE_H
