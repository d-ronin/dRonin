/**
 ******************************************************************************
 * @file       rgbleds.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup Modules Modules
 * @{
 * @addtogroup SystemModule System
 * @{
 *
 * @brief Implementation of annunciation and simple color blending for RGB LEDs.
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

#include "openpilot.h"

#include <pios_thread.h>

#include "rgbleds.h"
#include "misc_math.h"

#ifdef SYSTEMMOD_RGBLED_SUPPORT
#include "rgbledsettings.h"
#include "stabilizationdesired.h"
#include "manualcontrolcommand.h"

// all values are from 0 to 1
static void rgb_to_hsv_f(const float *rgb, float *hsv)
{
	float r = rgb[0];
	float g = rgb[1];
	float b = rgb[2];

	float h;

	float min_val, max_val, delta;

	uint8_t sector;

	if (r > g) {
		if (g < b) {
			min_val = g;
		} else {
			min_val = b;
		}

		if (r > b) {
			max_val = r;

			delta = max_val - min_val;

			// between yellow & magenta

			if (g >= b) {
				sector = 0;
			} else {
				sector = 6;
			}

			h = g - b;	
		} else {
			max_val = b;

			delta = max_val - min_val;

			// between magenta & cyan
			sector = 4;
			h = r - g;
		}
	} else {
		if (r < b) {
			min_val = r;
		} else {
			min_val = b;
		}

		if (g > b) {
			max_val = g;

			delta = max_val - min_val;

			// between cyan & yellow
			sector = 2;
			h = b - r;
		} else {
			max_val = b;

			delta = max_val - min_val;

			// between magenta & cyan
			sector = 4;
			h = r - g;
		}
	}

	if (delta <= 0.00001f) {
		hsv[0] = 0;		// hue undefined-- but pick 0 arbitrarily
		hsv[1] = 0;		// saturation 0
		hsv[2] = max_val;	// and brightness

		return;
	}

	h = (h / delta) + sector;

	hsv[0] = h * (1.0f / 6.0f);
	hsv[1] = delta / max_val;
	hsv[2] = max_val;
}

static void hsv_to_rgb_f(const float *hsv, float *rgb)
{
	float h = hsv[0], s = hsv[1], v = hsv[2];

	int i;
	float f, p, q, t;
	if (s < 0.000001f) {
		// achromatic (grey)

		rgb[0] = v;
		rgb[1] = v;
		rgb[2] = v;

		return;
	}

	h *= 6;				// sector 0 to 5
	i = h;

	f = h - i;			// fractional part of h

	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch (i) {
		case 0:
		case 6:
			rgb[0] = v;
			rgb[1] = t;
			rgb[2] = p;
			break;
		case 1:
			rgb[0] = q;
			rgb[1] = v;
			rgb[2] = p;
			break;
		case 2:
			rgb[0] = p;
			rgb[1] = v;
			rgb[2] = t;
			break;
		case 3:
			rgb[0] = p;
			rgb[1] = q;
			rgb[2] = v;
			break;
		case 4:
			rgb[0] = t;
			rgb[1] = p;
			rgb[2] = v;
			break;
		default:		// case 5:
			rgb[0] = v;
			rgb[1] = p;
			rgb[2] = q;
			break;
	}
}

static uint8_t float_to_q8(float f) {
	if (f <= 0) {
		return 0;
	}

	if (f >= 1) {
		return 255;
	}

	return 255 * f + 0.5f;
}

static inline uint8_t linear_interp_u16(uint8_t val_a, uint8_t val_b,
		uint16_t fraction)
{
	uint32_t tmp = ((uint32_t) val_a) * (65535-fraction);
	tmp += ((uint32_t) val_b) * fraction;

	tmp += 32767;

	return tmp >> 16;
}

static void interp_in_hsv(bool backwards, const uint8_t *rgb_start,
		const uint8_t *rgb_end, uint8_t *rgb_out, uint16_t fraction) {
	float fraction_f = fraction / 65535.0f;

	if (fraction_f > 1.0f) {
		fraction_f = 1.0f;
	}

	// Convert everything to HSV coordinates.
	float rgbf[3];

	rgbf[0] = rgb_start[0] / 255.0f;
	rgbf[1] = rgb_start[1] / 255.0f;
	rgbf[2] = rgb_start[2] / 255.0f;

	float hsv_start[3], hsv_end[3], hsv_now[3];

	rgb_to_hsv_f(rgbf, hsv_start);

	rgbf[0] = rgb_end[0] / 255.0f;
	rgbf[1] = rgb_end[1] / 255.0f;
	rgbf[2] = rgb_end[2] / 255.0f;

	rgb_to_hsv_f(rgbf, hsv_end);

	if (backwards) {
		if (hsv_start[0] > hsv_end[0]) {
			hsv_end[0] += 1.0f;
		} else {
			hsv_start[0] += 1.0f;
		}
	}

	hsv_now[0] = hsv_start[0] * (1.0f - fraction_f) +
		hsv_end[0] * (fraction_f);
	hsv_now[1] = hsv_start[1] * (1.0f - fraction_f) +
		hsv_end[1] * (fraction_f);
	hsv_now[2] = hsv_start[2] * (1.0f - fraction_f) +
		hsv_end[2] * (fraction_f);

	if (hsv_now[0] >= 1.0f) {
		hsv_now[0] -= 1.0f;
	}

	hsv_to_rgb_f(hsv_now, rgbf);

	rgb_out[0] = float_to_q8(rgbf[0]);
	rgb_out[1] = float_to_q8(rgbf[1]);
	rgb_out[2] = float_to_q8(rgbf[2]);
}

static inline uint16_t float_to_u16(float in)
{
	if (in >= 1) {
		return 65535;
	} else if (in <= 0) {
		return 0;
	}

	return in * 65535;
}

static uint16_t accessory_desired_get_u16(int idx)
{
	if (idx < 0) {
		return 0;
	}

	if (idx >= MANUALCONTROLCOMMAND_ACCESSORY_NUMELEM) {
		return 0;
	}

	float accessories[MANUALCONTROLCOMMAND_ACCESSORY_NUMELEM];

	ManualControlCommandAccessoryGet(accessories);

	return float_to_u16(accessories[idx]);
}

static uint16_t sinusodialize(uint16_t fraction) {
	int16_t s = sin_approx(fraction/2);

	if (s > 4095) return 65535;
	if (s < -4096) return 0;

	return (s + 4096) * 8;
}

void systemmod_process_rgb_leds(bool led_override, bool led_override_active,
		uint8_t blink_prio, bool is_armed, bool force_dim) {
	if (!pios_ws2811) return;

	uint16_t num_leds = PIOS_WS2811_get_num_leds(pios_ws2811);

	if (num_leds == 0) return;

	RGBLEDSettingsData rgbSettings;
	RGBLEDSettingsGet(&rgbSettings);

	uint8_t range_color[3];

	uint16_t fraction = 0;

	uint32_t tmpui32;
	float tmp_float;

	RGBLEDSettingsRangeColorBlendSourceOptions blend_source =
		rgbSettings.RangeColorBlendSource;
	RGBLEDSettingsRangeColorBlendModeOptions blend_mode =
		rgbSettings.RangeColorBlendMode;

	if (!is_armed) {
		blend_source = rgbSettings.RangeColorBlendUnarmedSource;
	}

	// future: would be nice to have per-index variance too for "motion"
	switch (blend_source) {
		case RGBLEDSETTINGS_RANGECOLORBLENDSOURCE_ALWAYSUSEBASECOLOR:
			break;

		case RGBLEDSETTINGS_RANGECOLORBLENDSOURCE_TIMEHALFSECONDPERIOD:
			tmpui32 = PIOS_Thread_Systime();
			fraction = (tmpui32 % 500) * 65535 / 500;
			if (blend_mode == RGBLEDSETTINGS_RANGECOLORBLENDMODE_SAWTOOTH) {
				/* Force to sinusodial modes for time */
				blend_mode = RGBLEDSETTINGS_RANGECOLORBLENDMODE_SINE;
			}
			break;

		case RGBLEDSETTINGS_RANGECOLORBLENDSOURCE_TIMESECONDPERIOD:
			tmpui32 = PIOS_Thread_Systime();
			fraction = (tmpui32 % 1000) * 65535 / 1000;
			if (blend_mode == RGBLEDSETTINGS_RANGECOLORBLENDMODE_SAWTOOTH) {
				/* Force to sinusodial modes for time */
				blend_mode = RGBLEDSETTINGS_RANGECOLORBLENDMODE_SINE;
			}
			break;
		
		case RGBLEDSETTINGS_RANGECOLORBLENDSOURCE_TIME2SECONDPERIOD:
			tmpui32 = PIOS_Thread_Systime();
			fraction = (tmpui32 % 2000) * 65535 / 2000;
			if (blend_mode == RGBLEDSETTINGS_RANGECOLORBLENDMODE_SAWTOOTH) {
				/* Force to sinusodial modes for time */
				blend_mode = RGBLEDSETTINGS_RANGECOLORBLENDMODE_SINE;
			}
			break;

		case RGBLEDSETTINGS_RANGECOLORBLENDSOURCE_TIME4SECONDPERIOD:
			tmpui32 = PIOS_Thread_Systime();
			fraction = (tmpui32 % 4000) * 65535 / 4000;
			if (blend_mode == RGBLEDSETTINGS_RANGECOLORBLENDMODE_SAWTOOTH) {
				/* Force to sinusodial modes for time */
				blend_mode = RGBLEDSETTINGS_RANGECOLORBLENDMODE_SINE;
			}
			break;

		case RGBLEDSETTINGS_RANGECOLORBLENDSOURCE_THROTTLE:
			StabilizationDesiredThrustGet(&tmp_float);
			fraction = float_to_u16(fabsf(tmp_float));
			break;

		case RGBLEDSETTINGS_RANGECOLORBLENDSOURCE_ACCESSORY0:
			fraction = accessory_desired_get_u16(0);
			break;

		case RGBLEDSETTINGS_RANGECOLORBLENDSOURCE_ACCESSORY1:
			fraction = accessory_desired_get_u16(1);
			break;

		case RGBLEDSETTINGS_RANGECOLORBLENDSOURCE_ACCESSORY2:
			fraction = accessory_desired_get_u16(2);
			break;
	}

	switch (blend_mode) {
		default:
		case RGBLEDSETTINGS_RANGECOLORBLENDMODE_SAWTOOTH:
			break;	// Do nothing.  It's sawtooth already

		case RGBLEDSETTINGS_RANGECOLORBLENDMODE_SINE:
			fraction = sinusodialize(fraction);
			break;

		case RGBLEDSETTINGS_RANGECOLORBLENDMODE_TRIANGLE:
			if (fraction >= 32768) {
				// Relies on unsigned wraparound magic
				fraction = 65535 - fraction * 2;
			} else {
				fraction *= 2;
			}
			break;
		case RGBLEDSETTINGS_RANGECOLORBLENDMODE_SQUARE:
			if (fraction > 32768) {
				fraction = 65535;
			} else {
				fraction = 0;
			}
			break;
	}

	switch (rgbSettings.RangeColorBlendType) {
		default:
		case RGBLEDSETTINGS_RANGECOLORBLENDTYPE_LINEARRGBFADE:
			range_color[0] = linear_interp_u16(
					rgbSettings.RangeBaseColor[0],
					rgbSettings.RangeEndColor[0],
					fraction);
			range_color[1] = linear_interp_u16(
					rgbSettings.RangeBaseColor[1],
					rgbSettings.RangeEndColor[1],
					fraction);
			range_color[2] = linear_interp_u16(
					rgbSettings.RangeBaseColor[2],
					rgbSettings.RangeEndColor[2],
					fraction);
		case RGBLEDSETTINGS_RANGECOLORBLENDTYPE_LINEARINHSV:
			interp_in_hsv(false, rgbSettings.RangeBaseColor,
					rgbSettings.RangeEndColor,
					range_color,
					fraction);
			break;
		case RGBLEDSETTINGS_RANGECOLORBLENDTYPE_LINEARINHSVBACKWARDSHUE:
			interp_in_hsv(true, rgbSettings.RangeBaseColor,
					rgbSettings.RangeEndColor,
					range_color,
					fraction);
			break;
	}

	if (force_dim) {
		range_color[0] /= 2;
		range_color[1] /= 2;
		range_color[2] /= 2;
	}

	uint8_t alarm_color[3] = { 0, 0, 0 };

	if (led_override && led_override_active) {
		// pick a meaningful color based on prio
	
		if (blink_prio >= SYSTEMALARMS_ALARM_CRITICAL) {
			alarm_color[0] = 255; // RED
			alarm_color[1] = 32;
			alarm_color[2] = 32;
		} else if (blink_prio >= SYSTEMALARMS_ALARM_WARNING) {
			alarm_color[0] = 255; // YELLOW
			alarm_color[1] = 200;
			alarm_color[2] = 32;
		} else {
			alarm_color[0] = 32; // GREEN
			alarm_color[1] = 200;
			alarm_color[2] = 32;
		}
	}

	for (int i = 0; i < num_leds; i++) {
		if (led_override) {
			if ((i >= rgbSettings.AnnunciateRangeBegin) &&
					(i <= rgbSettings.AnnunciateRangeEnd)) {
				PIOS_WS2811_set(pios_ws2811, i,
					       alarm_color[0],
					       alarm_color[1],
					       alarm_color[2]);

				continue;
			}
		}

		if ((i >= rgbSettings.RangeBegin) &&
				(i <= rgbSettings.RangeEnd)) {
			PIOS_WS2811_set(pios_ws2811, i,
					range_color[0], range_color[1],
					range_color[2]);
			continue;
		}

		PIOS_WS2811_set(pios_ws2811, i,
				rgbSettings.DefaultColor[0],
				rgbSettings.DefaultColor[1],
				rgbSettings.DefaultColor[2]);
	}

#ifdef SYSTEMMOD_RGBLED_VIDEO_HACK
	extern volatile bool video_active;

	if (!video_active) {
		PIOS_WS2811_trigger_update(pios_ws2811);
	}
#else
	PIOS_WS2811_trigger_update(pios_ws2811);
#endif
}

#endif

/**
 * @}
 * @}
 */
