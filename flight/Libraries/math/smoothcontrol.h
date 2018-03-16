#ifndef SMOOTHCONTROL_H
#define SMOOTHCONTROL_H

// Defines the predictor time-limit so it doesn't go haywire.
// Chosem PPM frame timing for now.
#define SMOOTHCONTROL_TIMEBOMB						20.0f

// Defines ratio of inter-signal timing to run a prediction algorithm for.
#define SMOOTHCONTROL_DUTY_CYCLE					0.5f

// Controls steepness of prediction slope.
#define SMOOTHCONTROL_PREDICTOR_SLOPE				0.8f

// At which ratio to start the chamfer.
#define SMOOTHCONTROL_CHAMFER_START					0.75f

enum {
	// Bypass.
	SMOOTHCONTROL_NONE,

	// Chamfers the corners of the stairstepped control signal.
	// icee's idea.
	SMOOTHCONTROL_NORMAL,

	// Linear interpolate control input.
	SMOOTHCONTROL_LINEAR,
};

typedef struct smoothcontrol_state_internal* smoothcontrol_state;


void smoothcontrol_initialize(smoothcontrol_state *state);
void smoothcontrol_update_dT(smoothcontrol_state state, float dT);
void smoothcontrol_next(smoothcontrol_state state);
void smoothcontrol_run(smoothcontrol_state state, uint8_t axis_num, float *new_signal);
void smoothcontrol_run_thrust(smoothcontrol_state state, float *new_signal);
void smoothcontrol_reinit(smoothcontrol_state state, uint8_t axis_num, float new_signal);
void smoothcontrol_reinit_thrust(smoothcontrol_state state, float new_signal);
void smoothcontrol_set_mode(smoothcontrol_state state, uint8_t axis_num, uint8_t mode, uint8_t duty_cycle);
bool* smoothcontrol_get_ringer(smoothcontrol_state state);

#endif // SMOOTHCONTROL_H
