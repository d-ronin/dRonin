/**
 ******************************************************************************
 * @file       ObjectFieldMappable.java
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      Support class for mapping between UAVO fields and controls
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

package org.dronin.androidgcs.util;

public interface ObjectFieldMappable {
	public double getValue();
	public void setValue(Number val);
	public void setOnChangedListener(Runnable run);
	public void setValue(String val);
}
