#ifndef _CHAROSD_H
#define _CHAROSD_H

#include "pios_max7456.h"
#include "charonscreendisplaysettings.h"

typedef struct {
	max7456_dev_t dev;
} * charosd_state_t;

typedef void (*update_t) (charosd_state_t state, uint8_t x, uint8_t y);

typedef struct
{
	update_t update;
} panel_t;

// panels collection
extern const panel_t panels [];
extern const uint8_t panels_count;

#endif // _CHAROSD_H
