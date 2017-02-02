/**
 ******************************************************************************
 *
 * @file       opmap_statusbar_widget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup OPMapPlugin Tau Labs Map Plugin
 * @{
 * @brief Tau Labs map plugin
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
 */

#include "opmap_statusbar_widget.h"
#include "ui_opmap_statusbar_widget.h"

opmap_statusbar_widget::opmap_statusbar_widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::opmap_statusbar_widget)
{
    ui->setupUi(this);
}

opmap_statusbar_widget::~opmap_statusbar_widget()
{
    delete ui;
}
