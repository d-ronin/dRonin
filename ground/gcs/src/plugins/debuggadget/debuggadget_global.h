/**
 ******************************************************************************
 * @file       debuggadget_global.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup DebugGadgetPlugin Debug Gadget Plugin
 * @{
 * @brief Handle debug messages from QDebug and place them in a widget
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

#ifndef DEBUGGADGET_GLOBAL_H
#define DEBUGGADGET_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(DEBUGGADGET_LIBRARY)
#define DEBUGGADGET_EXPORT Q_DECL_EXPORT
#else
#define DEBUGGADGET_EXPORT Q_DECL_IMPORT
#endif

#endif // DEBUGGADGET_GLOBAL_H

/**
 * @}
 * @}
 */
