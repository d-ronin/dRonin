/**
 ******************************************************************************
 *
 * @file       generator_io.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      I/O Code for the generator
 *
 * @see        The GNU Public License (GPL) Version 3
 *
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


#ifndef GENERATORIO
#define GENERATORIO

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <iostream>

QString readFile(QString name);
QString readFile(QString name);
bool writeFile(QString name, QString& str);
bool writeFileIfDiffrent(QString name, QString& str);

#endif
