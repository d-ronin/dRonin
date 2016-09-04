/*
 * This file is part of MultiOSD <https://github.com/UncleRus/MultiOSD>
 *
 * MultiOSD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef DEFAULTS_H_
#define DEFAULTS_H_

#ifdef ECLIPSE_BUILD
	// This settings used in Eclipse
	#define DEBUG
	//#define TELEMETRY_MODULES_UAVTALK
	#define TELEMETRY_MODULES_MSP
	//#define TELEMETRY_MODULES_ADC_BATTERY
	//#define TELEMETRY_MODULES_ADC_RSSI
	//#define TELEMETRY_MODULES_MAVLINK
	//#define TELEMETRY_MODULES_UBX
#else
	#if !defined (TELEMETRY_MODULES_UAVTALK) \
			&& !defined (TELEMETRY_MODULES_MAVLINK) \
			&& !defined (TELEMETRY_MODULES_UBX) \
			&& !defined (TELEMETRY_MODULES_MSP)
		#define TELEMETRY_MODULES_MAVLINK                // ArduPilot/ArduCopter telemetry module by default
	#endif
#endif

#if defined (TELEMETRY_MODULES_ADC_BATTERY) || defined (TELEMETRY_MODULES_ADC_BATTERY)
	#define ADC_MODULE
#endif

#define _VER(h,l) (((h) << 8) | l)


#endif /* DEFAULTS_H_ */
