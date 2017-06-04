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
 * with this program; if not, see <http://www.gnu.org/licenses/>
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

    /* For now assuming flyingpio bank configuration.  This may not hold
     * true in the long term...
     */
    channelBanks.resize(2);
    channelBanks[0] = QVector<int>() << 1 << 2; // TIM3
    channelBanks[1] = QVector<int>() << 3 << 4 << 5 << 6; // TIM1
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
    switch (capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_MAGS:
    case BOARD_CAPABILITIES_BAROS:
        return true;
    default:
        return false;
    }
    return false;
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

int Simulation::queryMaxGyroRate()
{
    /* Correct for flyingpi and reasonable for simulation. */
    return 2000;
}

bool Simulation::hasAnnunciator(AnnunciatorType annunc)
{
    switch (annunc) {
    case ANNUNCIATOR_HEARTBEAT:
    case ANNUNCIATOR_ALARM:
        return true;
    case ANNUNCIATOR_BUZZER:
    case ANNUNCIATOR_RGB:
    case ANNUNCIATOR_DAC:
        break;
    }
    return false;
}

/**
 * @}
 * @}
 */
