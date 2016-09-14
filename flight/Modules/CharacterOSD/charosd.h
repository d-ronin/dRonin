#ifndef _CHAROSD_H
#define _CHAROSD_H

#include "pios_max7456.h"
#include "charonscreendisplaysettings.h"

#include "gpsposition.h"
#include "flightbatterystate.h"

#define LINE_WIDTH 30
#define CENTER_STR(s) ((LINE_WIDTH - strlen(s)) / 2)

typedef struct {
	FlightBatteryStateData battery;
	struct {
		float roll, pitch, down;
	} actual;
	GPSPositionData gps_position;
	struct {
		int16_t rssi;
		float throttle;
	} manual;
	struct {
		uint8_t arm_status;
		uint8_t mode;
	} flight_status;
	struct {
		uint32_t flight_time;
	} system;
} telemetry_t;

typedef struct {
	max7456_dev_t dev;

	telemetry_t telemetry;
	char *custom_text;
	uint8_t prev_font;
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
