/**
 ******************************************************************************
 *
 * @file       testplugin_global.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief      
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   
 * @{
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

#ifndef TESTPLUGIN_GLOBAL_H
#define TESTPLUGIN_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(MYPLUGIN_LIBRARY)
#  define MYPLUGIN_EXPORT Q_DECL_EXPORT
#else
#  define MYPLUGIN_EXPORT Q_DECL_IMPORT
#endif

#endif // TESTPLUGIN_GLOBAL_H
