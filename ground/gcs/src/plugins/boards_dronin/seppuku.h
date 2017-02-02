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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef SEPPUKU_H_
#define SEPPUKU_H_

#include <uavobjectmanager.h>
#include <coreplugin/iboardtype.h>

class Seppuku : public Core::IBoardType
{
public:
    Seppuku();
    virtual ~Seppuku();

    virtual QString shortName();
    virtual QString boardDescription();
    virtual bool queryCapabilities(BoardCapabilities capability);
    virtual QPixmap getBoardPicture();
    virtual QString getHwUAVO();
    virtual bool isInputConfigurationSupported(Core::IBoardType::InputType type);
    virtual bool setInputType(Core::IBoardType::InputType type);
    virtual Core::IBoardType::InputType getInputType();
    virtual int queryMaxGyroRate();
    virtual QStringList getAdcNames();
    virtual QString getConnectionDiagram();
    virtual QWidget *getBoardConfiguration(QWidget *parent, bool connected);

private:
    UAVObjectManager *uavoManager;
};

#endif // SEPPUKU_H_

/**
 * @}
 * @}
 */
