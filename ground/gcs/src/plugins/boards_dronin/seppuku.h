/**
 ******************************************************************************
 * @file       seppuku.h
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

#ifndef SEPPUKU_H_
#define SEPPUKU_H_

#include <uavobjectmanager.h>
#include <coreplugin/iboardtype.h>

class IBoardType;

class Seppuku : public Core::IBoardType
{
public:
    Seppuku();
    virtual ~Seppuku();

    QString shortName();
    QString boardDescription();
    bool queryCapabilities(BoardCapabilities capability);
    QPixmap getBoardPicture();
    QString getHwUAVO();
    bool isInputConfigurationSupported(InputType type);
    bool setInputType(InputType type);
    enum InputType getInputType();
    int queryMaxGyroRate();
    QStringList getAdcNames();
    QString getConnectionDiagram();

private:
    UAVObjectManager *uavoManager;
};

#endif // SEPPUKU_H_

/**
 * @}
 * @}
 */
