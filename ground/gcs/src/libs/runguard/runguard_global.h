/**
 ******************************************************************************
 * @file       runguard_global.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup libs GCS Libraries
 * @{
 * @addtogroup runguard RunGuard
 * @{
 * @brief Provides a mechanism to ensure only one instance runs
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

#ifndef RUNGUARD_GLOBAL_H
#define RUNGUARD_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(RUNGUARD_LIB)
#  define RUNGUARD_EXPORT Q_DECL_EXPORT
#elif  defined(RUNGUARD_STATIC_LIB)
#  define RUNGUARD_EXPORT
#else
#  define RUNGUARD_EXPORT Q_DECL_IMPORT
#endif

#endif // RUNGUARD_GLOBAL_H
