/**
 ******************************************************************************
 * @file       simulationconfiguration.h
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

#ifndef SIMULATIONCONFIGURATION_H_
#define SIMULATIONCONFIGURATION_H_

#include <QPixmap>
#include "configtaskwidget.h"

namespace Ui {
class SimulationConfiguration;
}

class SimulationConfiguration : public ConfigTaskWidget
{
    Q_OBJECT

public:
    explicit SimulationConfiguration(QWidget *parent = 0);
    ~SimulationConfiguration();

private slots:
    void onLedStateUpdated(UAVObject *obj);

private:
    Ui::SimulationConfiguration *ui;
    QPixmap img;
};

#endif // SIMULATIONCONFIGURATION_H_

/**
 * @}
 * @}
 */
