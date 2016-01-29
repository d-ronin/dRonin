/**
 ******************************************************************************
 * @file       luxconfiguration.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
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


#ifndef LUXCONFIGURATION_H
#define LUXCONFIGURATION_H

#include <QPixmap>
#include "configtaskwidget.h"

namespace Ui {
class LuxConfiguration;
}

class LuxConfiguration : public ConfigTaskWidget
{
    Q_OBJECT
    
public:
    explicit LuxConfiguration(QWidget *parent = 0);
    ~LuxConfiguration();

private:
    Ui::LuxConfiguration *ui;

    QPixmap img;
};

#endif // LUXCONFIGURATION_H
