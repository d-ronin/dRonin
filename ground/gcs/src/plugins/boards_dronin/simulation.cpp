/**
 ******************************************************************************
 * @file       simulation.cpp
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

#include "simulation.h"

#include "hwsimulation.h"
#include "simulationconfiguration.h"

/**
 * @brief Simulation:Simulation
 *  This is the Simulation board definition
 */
Simulation::Simulation(void)
{
    boardType = 0x7F;
}

Simulation::~Simulation()
{
}

QString Simulation::shortName()
{
    return QString("Simulation");
}

QString Simulation::boardDescription()
{
    return QString("POSIX Simulator");
}

bool Simulation::queryCapabilities(BoardCapabilities capability)
{
    Q_UNUSED(capability);
    return false;
}

QStringList Simulation::getSupportedProtocols()
{
    return QStringList("uavtalk");
}

QPixmap Simulation::getBoardPicture()
{
    return QPixmap(":/images/gcs_logo_256.png");
}

QString Simulation::getHwUAVO()
{
    return "HwSimulation";
}

QWidget *Simulation::getBoardConfiguration(QWidget *parent, bool connected)
{
    Q_UNUSED(connected);
    return new SimulationConfiguration(parent);
}

/**
 * @}
 * @}
 */
