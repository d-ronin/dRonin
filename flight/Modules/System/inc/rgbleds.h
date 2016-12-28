#ifndef _RGBLEDS_H
#define _RGBLEDS_H

#include <stdint.h>

void systemmod_process_rgb_leds(bool led_override, bool led_override_active,
		uint8_t blink_prio, bool is_armed, bool force_dim);

#endif
