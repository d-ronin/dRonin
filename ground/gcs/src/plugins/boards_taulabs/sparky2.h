/**
 ******************************************************************************
 * @file       sparky2.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_TauLabsPlugin Tau Labs boards support Plugin
 * @{
 * @brief Plugin to support boards by the Tau Labs project
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
#ifndef SPARKY2_H
#define SPARKY2_H

#include "hwsparky2.h"
#include <coreplugin/iboardtype.h>
#include <uavobjectutil/uavobjectutilmanager.h>

class Sparky2 : public Core::IBoardType
{
public:
    Sparky2();
    virtual ~Sparky2();

    QString shortName();
    QString boardDescription();
    bool queryCapabilities(BoardCapabilities capability);
    QPixmap getBoardPicture();
    QString getHwUAVO();
    HwSparky2 * getSettings();

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
    QString getConnectionDiagram() { return ":/taulabs/images/sparky-connection-diagram.svg"; }

    int queryMaxGyroRate();

    /**
     * Get the RFM22b device ID this modem
     * @return RFM22B device ID or 0 if not supported
     */
    quint32 getRfmID();

    /**
     * Set the coordinator ID. If set to zero this device will
     * be a coordinator.
     * @return true if successful or false if not
     */
    bool bindRadio(quint32 id, quint32 baud_rate, float rf_power,
                           Core::IBoardType::LinkMode linkMode, quint8 min, quint8 max);

    QStringList getAdcNames();

private:
    UAVObjectUtilManager* uavoUtilManager;
};


#endif // SPARKY2_H
