/**
 ******************************************************************************
 * @addtogroup Modules Modules
 * @{
 * @addtogroup UAVOMSPBridge UAVO to MSP Bridge Module
 * @{
 *
 * @file       uavomspbridge.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2017
 * @brief      Bridges selected UAVObjects to MSP for MWOSD and the like.
 *
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "openpilot.h"
#include "misc_math.h"
#include "physical_constants.h"
#include "pios_thread.h"
#include "pios_sensors.h"
#include "pios_modules.h"
#include <pios_hal.h>

#include "actuatorsettings.h"
#include "actuatordesired.h"
#include "airspeedactual.h"
#include "attitudeactual.h"
#include "baroaltitude.h"
#include "flightbatterysettings.h"
#include "flightbatterystate.h"
#include "flightstatus.h"
#include "gpsposition.h"
#include "homelocation.h"
#include "manualcontrolcommand.h"
#include "modulesettings.h"
#include "positionactual.h"
#include "stabilizationsettings.h"
#include "systemalarms.h"
#include "systemsettings.h"
#include "systemstats.h"


#if defined(PIOS_INCLUDE_MSP_BRIDGE)

#define MSP_SENSOR_ACC 1
#define MSP_SENSOR_BARO 2
#define MSP_SENSOR_MAG 4
#define MSP_SENSOR_GPS 8

#define  MSP_API_VERSION 1
#define  MSP_FC_VARIANT  2
#define  MSP_FC_VERSION  3
#define  MSP_BOARD_INFO  4
#define  MSP_BUILD_INFO2 5   // clean/betaflight extension :( (MSP_BUILD_INFO had room for a 28-bit hash + up to 36-bit time already!)
#define  MSP_NAME        10
#define  MSP_FEATURE     36
#define  MSP_CONFIG      66  // baseflight
#define  MSP_BUILD_INFO  69  // baseflight
#define  MSP_IDENT       100 // multitype + multiwii version + protocol version + capability variable
#define  MSP_STATUS      101 // cycletime & errors_count & sensor present & box activation & current setting number
#define  MSP_RAW_IMU     102 // 9 DOF
#define  MSP_SERVO       103 // 8 servos
#define  MSP_MOTOR       104 // 8 motors
#define  MSP_RC          105 // 8 rc chan and more
#define  MSP_RAW_GPS     106 // fix, numsat, lat, lon, alt, speed, ground course
#define  MSP_COMP_GPS    107 // distance home, direction home
#define  MSP_ATTITUDE    108 // 2 angles 1 heading
#define  MSP_ALTITUDE    109 // altitude, variometer
#define  MSP_ANALOG      110 // vbat, powermetersum, rssi if available on RX
#define  MSP_RC_TUNING   111 // rc rate, rc expo, rollpitch rate, yaw rate, dyn throttle PID
#define  MSP_PID         112 // P I D coeff (9 are used currently)
#define  MSP_BOX         113 // BOX setup (number is dependant of your setup)
#define  MSP_MISC        114 // powermeter trig
#define  MSP_MOTOR_PINS  115 // which pins are in use for motors & servos, for GUI
#define  MSP_BOXNAMES    116 // the aux switch names
#define  MSP_PIDNAMES    117 // the PID names
#define  MSP_BOXIDS      119 // get the permanent IDs associated to BOXes
#define  MSP_NAV_STATUS  121 // Returns navigation status
#define  MSP_CELLS       130 // FrSky SPort Telemtry
#define  MSP_UID         160 // Hardware serial number
#define  MSP_SET_PID     202
#define  MSP_ALARMS      242 // Alarm request
#define  MSP_SET_4WAY_IF 245 // Sets ESC serial interface

#define MSP_PROTOCOL_VERSION  0
#define MSP_API_VERSION_MAJOR 1
#define MSP_API_VERSION_MINOR 31 // Matches 3.1.6

enum msp_handler {
	MSP_HANDLER_MSP = 0,
	MSP_HANDLER_IDLE,
	MSP_HANDLER_4WIF
};

enum msp_vehicle_type {
	MSP_MULTITYPE_TRI = 1,
	MSP_MULTITYPE_QUADP = 2,
	MSP_MULTITYPE_QUADX = 3,
	MSP_MULTITYPE_BI = 4,
	MSP_MULTITYPE_GIMBAL = 5,
	MSP_MULTITYPE_Y6 = 6,
	MSP_MULTITYPE_HEX6 = 7,
	MSP_MULTITYPE_FLYING_WING = 8,
	MSP_MULTITYPE_Y4 = 9,
	MSP_MULTITYPE_HEX6X = 10,
	MSP_MULTITYPE_OCTOX8 = 11,
	MSP_MULTITYPE_OCTOFLATP = 12,
	MSP_MULTITYPE_OCTOFLATX = 13,
	MSP_MULTITYPE_AIRPLANE = 14,
	MSP_MULTITYPE_HELI_120_CCPM = 15,
	MSP_MULTITYPE_HELI_90_DEG = 16,
	MSP_MULTITYPE_VTAIL4 = 17,
	MSP_MULTITYPE_HEX6H = 18,
	MSP_MULTITYPE_PPM_TO_SERVO = 19,
	MSP_MULTITYPE_DUALCOPTER = 20,
	MSP_MULTITYPE_SINGLECOPTER = 21,
	MSP_MULTITYPE_ATAIL4 = 22,
	MSP_MULTITYPE_CUSTOM = 23,
	MSP_MULTITYPE_CUSTOM_PLANE = 24,
};

typedef enum {
	MSP_BOX_ARM,
	MSP_BOX_ANGLE,
	MSP_BOX_HORIZON,
	MSP_BOX_BARO,
	MSP_BOX_VARIO,
	MSP_BOX_MAG,
	MSP_BOX_GPSHOME,
	MSP_BOX_GPSHOLD,
	MSP_BOX_LAST,
} msp_box_t;

const static struct {
	msp_box_t mode;
	uint8_t mwboxid;
	FlightStatusFlightModeOptions tlmode;
} msp_boxes[] = {
	{ MSP_BOX_ARM, 0, 0 },
	{ MSP_BOX_ANGLE, 1, FLIGHTSTATUS_FLIGHTMODE_LEVELING},
	{ MSP_BOX_HORIZON, 2, FLIGHTSTATUS_FLIGHTMODE_HORIZON},
	{ MSP_BOX_BARO, 3, FLIGHTSTATUS_FLIGHTMODE_ALTITUDEHOLD},
	{ MSP_BOX_VARIO, 4, 0},
	{ MSP_BOX_MAG, 5, 0},
	{ MSP_BOX_GPSHOME, 10, FLIGHTSTATUS_FLIGHTMODE_RETURNTOHOME},
	{ MSP_BOX_GPSHOLD, 11, FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD},
	{ MSP_BOX_LAST, 0xff, 0},
};

typedef enum {
	MSP_IDLE,
	MSP_HEADER_START,
	MSP_HEADER_M,
	MSP_HEADER_SIZE,
	MSP_HEADER_CMD,
	MSP_FILLBUF,
	MSP_CHECKSUM,
	MSP_DISCARD,
	MSP_MAYBE_UAVTALK2,
	MSP_MAYBE_UAVTALK3,
	MSP_MAYBE_UAVTALK4,
} msp_state;

typedef enum __attribute__ ((__packed__)) {
	PROTOCOL_SIMONK = 0,
	PROTOCOL_BLHELI = 1,
	PROTOCOL_KISS = 2,
	PROTOCOL_KISSALL = 3,
	PROTOCOL_CASTLE = 4,
	PROTOCOL_4WAY = 0xff,
} msp_esc_protocol;

struct msp_cmddata_escserial {
	msp_esc_protocol protocol;
	uint8_t esc_num;
};

struct msp_bridge {
	uintptr_t com;

	enum msp_handler handler;
	msp_state state;
	uint8_t cmd_size;
	uint8_t cmd_id;
	uint8_t cmd_i;
	uint8_t checksum;
	union {
		uint8_t data[128];
		// Specific packed data structures go here.
		struct msp_cmddata_escserial escserial;
	} cmd_data;
};

#if defined(PIOS_MSP_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_MSP_STACK_SIZE
#else
#define STACK_SIZE_BYTES 1500
#endif
#define TASK_PRIORITY               PIOS_THREAD_PRIO_LOW

#define MAX_ALARM_LEN 30

#define BOOT_DISPLAY_TIME_MS (10*1000)

static bool module_enabled = false;
extern uintptr_t pios_com_msp_id;
static struct msp_bridge *msp;
static int32_t uavoMSPBridgeInitialize(void);
static void uavoMSPBridgeTask(void *parameters);
void esc4wayProcess(void *mspPort);


static void msp_send_error(struct msp_bridge *m, uint8_t cmd)
{
	uint8_t buf[6];

	buf[0] = '$';
	buf[1] = 'M';
	buf[2] = '!'; /* original multiwii spec says '|', everyone uses '!' */
	buf[3] = 0;
	buf[4] = cmd;
	buf[5] = cmd;	// Checksum == cmd

	PIOS_COM_SendBuffer(m->com, buf, sizeof(buf));
}

static void msp_send(struct msp_bridge *m, uint8_t cmd, const uint8_t *data, size_t len)
{
	uint8_t buf[5];
	uint8_t cs = (uint8_t)(len) ^ cmd;

	buf[0] = '$';
	buf[1] = 'M';
	buf[2] = '>';
	buf[3] = (uint8_t)(len);
	buf[4] = cmd;

	PIOS_COM_SendBuffer(m->com, buf, sizeof(buf));
	PIOS_COM_SendBuffer(m->com, data, len);

	for (int i = 0; i < len; i++) {
		cs ^= data[i];
	}
	cs ^= 0;

	buf[0] = cs;
	PIOS_COM_SendBuffer(m->com, buf, 1);
}

static msp_state msp_state_size(struct msp_bridge *m, uint8_t b)
{
	m->cmd_size = b;
	m->checksum = b;
	return MSP_HEADER_CMD;
}

static msp_state msp_state_cmd(struct msp_bridge *m, uint8_t b)
{
	m->cmd_i = 0;
	m->cmd_id = b;
	m->checksum ^= m->cmd_id;

	if (m->cmd_size > sizeof(m->cmd_data)) {
		// Too large a body.  Let's ignore it.
		return MSP_DISCARD;
	}

	return m->cmd_size == 0 ? MSP_CHECKSUM : MSP_FILLBUF;
}

static msp_state msp_state_fill_buf(struct msp_bridge *m, uint8_t b)
{
	m->cmd_data.data[m->cmd_i++] = b;
	m->checksum ^= b;
	return m->cmd_i == m->cmd_size ? MSP_CHECKSUM : MSP_FILLBUF;
}

static void msp_send_name(struct msp_bridge *m)
{
	const char hardcoded_name[] = "dRonin";

	msp_send(m, MSP_NAME, (uint8_t *) hardcoded_name, strlen(hardcoded_name));
}

static void msp_send_motor(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		uint16_t mspeed[8];
	} data;

	/* Tell me lies */
	memset(&data, 0, sizeof(data));

	msp_send(m, MSP_MOTOR, data.buf, sizeof(data));
}

static void msp_send_feature(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		uint32_t features;
	} data;

#define FEATURE_TELEMETRY (1 << 10)
	data.features = FEATURE_TELEMETRY;

	msp_send(m, MSP_FEATURE, data.buf, sizeof(data));
}

static void msp_send_misc(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint16_t mid_rc;
			uint16_t min_throt;
			uint16_t max_throt;
			uint16_t min_command;
			uint8_t gps[3];
			uint8_t misc_cfg[3];
			uint16_t compass;
			uint8_t battery[4];
		} fc_misc;
	} data;

	/* Tell me sweet little lies */
	memset(&data, 0, sizeof(data));

	data.fc_misc.mid_rc = 1500;
	data.fc_misc.min_command = 1000;
	data.fc_misc.min_throt = 1150;
	data.fc_misc.max_throt = 2000;		// 1850 in BF...

	msp_send(m, MSP_MISC, data.buf, sizeof(data));
}

static void msp_send_fc_version(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint8_t major;
			uint8_t minor;
			uint8_t patch;
		} fc_ver;
	} data;

	/* Not very meaningful to us */
	data.fc_ver.major = 1;
	data.fc_ver.minor = 0;
	data.fc_ver.patch = 0;

	msp_send(m, MSP_FC_VERSION, data.buf, sizeof(data));
}

static void msp_send_fc_variant(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			char name[4];
		} var;
	} data;

	memset(&data, 0, sizeof(data));

	memcpy(data.var.name, "DRON", 4);

	msp_send(m, MSP_FC_VARIANT, data.buf, sizeof(data));
}

static void msp_send_board_info(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct __attribute__((packed)) {
			char name[4];
			uint16_t revision;
		} board;
	} data;

	memset(&data, 0, sizeof(data));

	memcpy(data.board.name, DRONIN_TARGET, MIN(4, strlen(DRONIN_TARGET)));

	msp_send(m, MSP_BOARD_INFO, data.buf, sizeof(data));
}

static void msp_send_config(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint8_t mixer_config;
			uint32_t features;
			uint8_t serial_rx;
			uint16_t align_roll;
			uint16_t align_pitch;
			uint16_t align_yaw;
			uint16_t current_scale;
			uint16_t current_offset;
			uint16_t motor_pwm_rate;
			uint8_t roll_pitch_rate[2];
			uint8_t power_adc_channel;
			uint8_t small_angle;
			uint16_t looptime;
			uint8_t locked_in;
		} __attribute__((packed)) config;
	} data;

	memset(&data, 0, sizeof(data));

	data.config.locked_in = 1;

	msp_send(m, MSP_CONFIG, data.buf, sizeof(data));
}

static void msp_send_build_info(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct __attribute__((packed)) {
			char date[11];
			uint32_t _future[2];
		} build;
	} data;

	memset(&data, 0, sizeof(data));

	// XXX: Not impl

	msp_send(m, MSP_BUILD_INFO, data.buf, sizeof(data));
}

static void msp_send_build_info_cf(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct __attribute__((packed)) {
			char date[11];
			char time[8];
			char short_rev[7];
		} build;
	} data;

	memset(&data, 0, sizeof(data));

	// XXX: Not impl

	msp_send(m, MSP_BUILD_INFO2, data.buf, sizeof(data));
}

DONT_BUILD_IF(PIOS_SYS_SERIAL_NUM_BINARY_LEN != 4*3, SerialNumberAssumption);

static void msp_send_uid(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint8_t serial[PIOS_SYS_SERIAL_NUM_BINARY_LEN];
		} uid;
	} data;

	PIOS_SYS_SerialNumberGetBinary(data.uid.serial);

	msp_send(m, MSP_UID, data.buf, sizeof(data));
}

static void msp_send_api_version(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint8_t protocol_ver;
			uint8_t api_version_major;
			uint8_t api_version_minor;
		} ident;
	} data;

	data.ident.protocol_ver = MSP_PROTOCOL_VERSION;
	data.ident.api_version_major = MSP_API_VERSION_MAJOR;
	data.ident.api_version_minor = MSP_API_VERSION_MINOR;

	msp_send(m, MSP_API_VERSION, data.buf, sizeof(data));
}

static void msp_send_attitude(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			int16_t x;
			int16_t y;
			int16_t h;
		} att;
	} data;
	AttitudeActualData attActual;

	AttitudeActualGet(&attActual);

	// Roll and Pitch are in 10ths of a degree.
	data.att.x = attActual.Roll * 10;
	data.att.y = attActual.Pitch * -10;
	// Yaw is just -180 -> 180
	data.att.h = attActual.Yaw;

	msp_send(m, MSP_ATTITUDE, data.buf, sizeof(data));
}

static void msp_send_ident(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint8_t multiwii_version;
			uint8_t vehicle_type;
			uint8_t msp_version;
			uint32_t capabilities;
		} __attribute__((packed)) ident;
	} data;

	data.ident.multiwii_version = 255;
	data.ident.msp_version = 4;
	data.ident.capabilities = 0x80000004; /* 32-bit | DYNBALANCE */

	uint8_t type;
	SystemSettingsAirframeTypeGet(&type);
	switch (type) {
	case SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWING:
		data.ident.vehicle_type = MSP_MULTITYPE_AIRPLANE;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGELEVON:
		data.ident.vehicle_type = MSP_MULTITYPE_FLYING_WING;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGVTAIL:
		data.ident.vehicle_type = MSP_MULTITYPE_CUSTOM_PLANE;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_VTOL:
		data.ident.vehicle_type = MSP_MULTITYPE_CUSTOM;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_HELICP:
		data.ident.vehicle_type = MSP_MULTITYPE_HELI_120_CCPM; /* maybe */
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_QUADX:
		data.ident.vehicle_type = MSP_MULTITYPE_QUADX;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_QUADP:
		data.ident.vehicle_type = MSP_MULTITYPE_QUADP;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_HEXA:
		data.ident.vehicle_type = MSP_MULTITYPE_HEX6H;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_OCTO:
		data.ident.vehicle_type = MSP_MULTITYPE_OCTOFLATX; /* ? */
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_CUSTOM:
		data.ident.vehicle_type = MSP_MULTITYPE_CUSTOM;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_HEXAX:
		data.ident.vehicle_type = MSP_MULTITYPE_HEX6X;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_OCTOV:
		data.ident.vehicle_type = MSP_MULTITYPE_CUSTOM;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_OCTOCOAXP:
		data.ident.vehicle_type = MSP_MULTITYPE_CUSTOM;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_OCTOCOAXX:
		data.ident.vehicle_type = MSP_MULTITYPE_OCTOX8; /* ? */
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_HEXACOAX:
		data.ident.vehicle_type = MSP_MULTITYPE_CUSTOM;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_TRI:
		data.ident.vehicle_type = MSP_MULTITYPE_TRI;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_GROUNDVEHICLECAR:
		data.ident.vehicle_type = MSP_MULTITYPE_CUSTOM;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_GROUNDVEHICLEDIFFERENTIAL:
		data.ident.vehicle_type = MSP_MULTITYPE_CUSTOM;
		break;
	case SYSTEMSETTINGS_AIRFRAMETYPE_GROUNDVEHICLEMOTORCYCLE:
		data.ident.vehicle_type = MSP_MULTITYPE_CUSTOM;
		break;
	}

	msp_send(m, MSP_IDENT, data.buf, sizeof(data));
}

static void msp_send_rc_tuning(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint8_t rc_rate;
			uint8_t rx_expo;
			uint8_t _roll_pitch_rate;
			uint8_t yaw_rate;
			uint8_t dyn_throttle_pid;
			uint8_t throttle_mid;
			uint8_t throttle_expo;
		} __attribute__((packed)) tune;
	} data;

	memset(&data, 0, sizeof(data));

	msp_send(m, MSP_RC_TUNING, data.buf, sizeof(data));
}

#define PID_SCALE_KP 10000.0f
#define PID_SCALE_KI 1000.0f
#define PID_SCALE_KD 1000000.0f

static void msp_set_pid(struct msp_bridge *m)
{
	struct set_pid {
		struct {
			uint8_t P;
			uint8_t I;
			uint8_t D;
		} __attribute__((packed)) axis[10];
	} __attribute__((packed));

	struct set_pid *data = (struct set_pid *)m->cmd_data.data;

	uint8_t armed;
	FlightStatusArmedGet(&armed);

	if (sizeof(*data) > m->cmd_i || armed != FLIGHTSTATUS_ARMED_DISARMED)
		msp_send_error(m, MSP_SET_PID);

	StabilizationSettingsData stab;
	StabilizationSettingsGet(&stab);

	#define SCALE_PID_DR(axisnum, pid) ((float)data->axis[axisnum].pid / PID_SCALE_K##pid)

	stab.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_KP] = SCALE_PID_DR(0, P);
	stab.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_KI] = SCALE_PID_DR(0, I);
	stab.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_KD] = SCALE_PID_DR(0, D);
	stab.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_KP] = SCALE_PID_DR(1, P);
	stab.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_KI] = SCALE_PID_DR(1, I);
	stab.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_KD] = SCALE_PID_DR(1, D);
	stab.YawRatePID[STABILIZATIONSETTINGS_YAWRATEPID_KP] = SCALE_PID_DR(2, P);
	stab.YawRatePID[STABILIZATIONSETTINGS_YAWRATEPID_KI] = SCALE_PID_DR(2, I);
	stab.YawRatePID[STABILIZATIONSETTINGS_YAWRATEPID_KD] = SCALE_PID_DR(2, D);

	StabilizationSettingsSet(&stab);

	msp_send(m, MSP_SET_PID, NULL, 0);
}

static void msp_send_pid(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			struct {
				uint8_t p;
				uint8_t i;
				uint8_t d;
			} __attribute__((packed)) axis[10];
		} __attribute__((packed)) pid;
	} data;

	memset(&data, 0, sizeof(data));

	StabilizationSettingsData stab;
	StabilizationSettingsGet(&stab);

	#define SCALE_PID_MW(axis, axisu, pid) bound_min_max( \
		stab.axis##RatePID[STABILIZATIONSETTINGS_##axisu##RATEPID_K##pid] \
		* PID_SCALE_K##pid, 0.0f, 255.0f)

	data.pid.axis[0].p = SCALE_PID_MW(Roll, ROLL, P);
	data.pid.axis[0].i = SCALE_PID_MW(Roll, ROLL, I);
	data.pid.axis[0].d = SCALE_PID_MW(Roll, ROLL, D);
	data.pid.axis[1].p = SCALE_PID_MW(Pitch, PITCH, P);
	data.pid.axis[1].i = SCALE_PID_MW(Pitch, PITCH, I);
	data.pid.axis[1].d = SCALE_PID_MW(Pitch, PITCH, D);
	data.pid.axis[2].p = SCALE_PID_MW(Yaw, YAW, P);
	data.pid.axis[2].i = SCALE_PID_MW(Yaw, YAW, I);
	data.pid.axis[2].d = SCALE_PID_MW(Yaw, YAW, D);

	msp_send(m, MSP_PID, data.buf, sizeof(data));
}

static void msp_send_pid_names(struct msp_bridge *m)
{
	const char names[] = "ROLL;PITCH;YAW;";
	msp_send(m, MSP_PIDNAMES, (const uint8_t *)names, sizeof(names));
}

static void msp_send_status(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint16_t cycleTime;
			uint16_t i2cErrors;
			uint16_t sensors;
			uint32_t flags;
			uint8_t setting;
		} __attribute__((packed)) status;
	} data;

	float cycle_time;
	ActuatorDesiredUpdateTimeGet(&cycle_time);
	data.status.cycleTime = cycle_time * 1000;

	data.status.i2cErrors = 0;
	
	GPSPositionData gpsData = {};
	
	if (GPSPositionHandle() != NULL)
		GPSPositionGet(&gpsData);

	data.status.sensors = (PIOS_SENSORS_IsRegistered(PIOS_SENSOR_ACCEL) ? MSP_SENSOR_ACC  : 0) |
		(PIOS_SENSORS_IsRegistered(PIOS_SENSOR_BARO) ? MSP_SENSOR_BARO : 0) |
		(PIOS_SENSORS_IsRegistered(PIOS_SENSOR_MAG) ? MSP_SENSOR_MAG : 0) |
		(gpsData.Status != GPSPOSITION_STATUS_NOGPS ? MSP_SENSOR_GPS : 0);

	data.status.flags = 0;
	data.status.setting = 0;

	if (FlightStatusHandle() != NULL) {
		FlightStatusData flight_status;
		FlightStatusGet(&flight_status);

		data.status.flags = flight_status.Armed == FLIGHTSTATUS_ARMED_ARMED;

		for (int i = 1; msp_boxes[i].mode != MSP_BOX_LAST; i++) {
			if (flight_status.FlightMode == msp_boxes[i].tlmode) {
				data.status.flags |= (1 << i);
			}
		}
	}

	msp_send(m, MSP_STATUS, data.buf, sizeof(data));
}

static void msp_send_analog(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint8_t vbat;
			uint16_t powerMeterSum;
			uint16_t rssi;
			uint16_t current;
		} __attribute__((packed)) status;
	} data;
	data.status.powerMeterSum = 0;

	FlightBatterySettingsData batSettings = {};
	FlightBatteryStateData batState = {};

	if (FlightBatteryStateHandle() != NULL)
		FlightBatteryStateGet(&batState);
	if (FlightBatterySettingsHandle() != NULL) {
		FlightBatterySettingsGet(&batSettings);
	}

	if (batSettings.VoltagePin != FLIGHTBATTERYSETTINGS_VOLTAGEPIN_NONE)
		data.status.vbat = (uint8_t)lroundf(batState.Voltage * 10);

	if (batSettings.CurrentPin != FLIGHTBATTERYSETTINGS_CURRENTPIN_NONE) {
		data.status.current = lroundf(batState.Current * 100);
		data.status.powerMeterSum = lroundf(batState.ConsumedEnergy);
	}

	ManualControlCommandData manualState;
	ManualControlCommandGet(&manualState);

	// MSP RSSI's range is 0-1023
	if (manualState.Rssi <= 0) {
		data.status.rssi = 0;
	} else if (manualState.Rssi >= 100) {
		data.status.rssi = 1023;
	} else {
		data.status.rssi = manualState.Rssi * 10;
	}

	msp_send(m, MSP_ANALOG, data.buf, sizeof(data));
}

static void msp_send_raw_gps(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint8_t  fix;                 // 0 or 1
			uint8_t  num_sat;
			int32_t lat;                  // 1 / 10 000 000 deg
			int32_t lon;                  // 1 / 10 000 000 deg
			uint16_t alt;                 // meter
			uint16_t speed;               // cm/s
			int16_t ground_course;        // degree * 10
		} __attribute__((packed)) raw_gps;
	} data;
	
	GPSPositionData gps_data = {};
	
	if (GPSPositionHandle() != NULL)
	{
		GPSPositionGet(&gps_data);
		data.raw_gps.fix           = (gps_data.Status >= GPSPOSITION_STATUS_FIX2D ? 1 : 0);  // Data will display on OSD if 2D fix or better
		data.raw_gps.num_sat       = gps_data.Satellites;
		data.raw_gps.lat           = gps_data.Latitude;
		data.raw_gps.lon           = gps_data.Longitude;
		data.raw_gps.alt           = (uint16_t)gps_data.Altitude;
		data.raw_gps.speed         = (uint16_t)(gps_data.Groundspeed * 100.0f);
		data.raw_gps.ground_course = (int16_t)(gps_data.Heading * 10.0f);
	}
	else
	{
		data.raw_gps.fix           = 0;  // Data won't display on OSD
		data.raw_gps.num_sat       = 0;
		data.raw_gps.lat           = 0;
		data.raw_gps.lon           = 0;
		data.raw_gps.alt           = 0;
		data.raw_gps.speed         = 0;
		data.raw_gps.ground_course = 0;
	}
	
	msp_send(m, MSP_RAW_GPS, data.buf, sizeof(data));
}

static void msp_send_comp_gps(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint16_t distance_to_home;     // meter
			int16_t  direction_to_home;    // degree [-180:180]
			uint8_t  home_position_valid;  // 0 = Invalid, Dronin MSP specific
		} __attribute__((packed)) comp_gps;
	} data;
	
	GPSPositionData gps_data   = {};
	HomeLocationData home_data = {};
	
	if ((GPSPositionHandle() == NULL) || (HomeLocationHandle() == NULL))
	{
		data.comp_gps.distance_to_home    = 0;
		data.comp_gps.direction_to_home   = 0;
		data.comp_gps.home_position_valid = 0;  // Home distance and direction will not display on OSD
	}
	else
	{
		GPSPositionGet(&gps_data);
		HomeLocationGet(&home_data);
		
		if((gps_data.Status < GPSPOSITION_STATUS_FIX2D) || (home_data.Set == HOMELOCATION_SET_FALSE))
		{
			data.comp_gps.distance_to_home    = 0;
			data.comp_gps.direction_to_home   = 0;
			data.comp_gps.home_position_valid = 0;  // Home distance and direction will not display on OSD
		}
		else
		{
			data.comp_gps.home_position_valid = 1;  // Home distance and direction will display on OSD
			
			int32_t delta_lon = (home_data.Longitude - gps_data.Longitude);  // degrees * 1e7
			int32_t delta_lat = (home_data.Latitude  - gps_data.Latitude );  // degrees * 1e7
	
			float delta_y = (float)delta_lon * WGS84_RADIUS_EARTH_KM * DEG2RAD;  // KM * 1e7
			float delta_x = (float)delta_lat * WGS84_RADIUS_EARTH_KM * DEG2RAD;  // KM * 1e7
	
			delta_y *= cosf((float)home_data.Latitude * 1e-7f * (float)DEG2RAD);  // Latitude compression correction
	
			data.comp_gps.distance_to_home  = (uint16_t)(sqrtf(delta_x * delta_x + delta_y * delta_y) * 1e-4f);  // meters
	
			if ((delta_lon == 0) && (delta_lat == 0))
				data.comp_gps.direction_to_home = 0;
			else
				data.comp_gps.direction_to_home = (int16_t)(atan2f(delta_y, delta_x) * RAD2DEG); // degrees;
		}			
	}

	msp_send(m, MSP_COMP_GPS, data.buf, sizeof(data));
}

static void msp_send_altitude(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			int32_t alt; // cm
			uint16_t vario; // cm/s
		} __attribute__((packed)) baro;
	} data;
	
	float tmp;

	if (PositionActualHandle() != NULL) {
		PositionActualDownGet(&tmp);
		tmp = -tmp;
	} else {
		return;
	}

	data.baro.alt = (int32_t)roundf(tmp * 100.0f);

	msp_send(m, MSP_ALTITUDE, data.buf, sizeof(data));
}

// Scale stick values whose input range is -1 to 1 to MSP's expected
// PWM range of 1000-2000.
static uint16_t msp_scale_rc(float percent) {
	return percent*500 + 1500;
}

// Throttle maps to 1100-1900 and a bit differently as -1 == 1000 and
// then jumps immediately to 0 -> 1 for the rest of the range.  MWOSD
// can learn ranges as wide as they are sent, but defaults to
// 1100-1900 so the throttle indicator will vary for the same stick
// position unless we send it what it wants right away.
static uint16_t msp_scale_rc_thr(float percent) {
	return percent <= 0 ? 1100 : percent*800 + 1100;
}

// MSP RC order is Roll/Pitch/Yaw/Throttle/AUX1/AUX2/AUX3/AUX4
static void msp_send_channels(struct msp_bridge *m)
{
	ManualControlCommandData manualState;
	ManualControlCommandGet(&manualState);

	union {
		uint8_t buf[0];
		uint16_t channels[8];
	} data = {
		.channels = {
			msp_scale_rc(manualState.Roll),
			msp_scale_rc(manualState.Pitch * -1), // MW pitch is backwards
			msp_scale_rc(manualState.Yaw),
			msp_scale_rc_thr(manualState.Throttle),
			msp_scale_rc(manualState.Accessory[0]),
			msp_scale_rc(manualState.Accessory[1]),
			msp_scale_rc(manualState.Accessory[2]),
			1000, // no aux4
		}
	};

	msp_send(m, MSP_RC, data.buf, sizeof(data));
}

static void msp_send_boxids(struct msp_bridge *m) {
	uint8_t boxes[MSP_BOX_LAST];
	int len = 0;

	for (int i = 0; msp_boxes[i].mode != MSP_BOX_LAST; i++) {
		boxes[len++] = msp_boxes[i].mwboxid;
	}
	msp_send(m, MSP_BOXIDS, boxes, len);
}

#ifdef PIOS_INCLUDE_4WAY
uint8_t esc4wayInit(void);
#endif

static void msp_handle_4wif(struct msp_bridge *m) {
	struct msp_cmddata_escserial *escserial = &m->cmd_data.escserial;

	uint8_t num_esc = 0;

	msp_esc_protocol protocol = PROTOCOL_4WAY;

	if (m->cmd_size >= sizeof(*escserial)) {
		protocol = escserial->protocol;
	}

	switch (protocol) {
		case PROTOCOL_4WAY:
#ifdef PIOS_INCLUDE_4WAY
			actuator_interlock = ACTUATOR_INTERLOCK_STOPREQUEST;

			while (actuator_interlock != ACTUATOR_INTERLOCK_STOPPED) {
				PIOS_Thread_Sleep(3);
			}

			num_esc = esc4wayInit();
			m->handler = MSP_HANDLER_4WIF;
#endif
			break;

		case PROTOCOL_SIMONK:
		case PROTOCOL_BLHELI:
		case PROTOCOL_KISS:
		case PROTOCOL_KISSALL:
		case PROTOCOL_CASTLE:
		default:
			/* Unsupported protocol */
			break;
	}

	msp_send(m, MSP_SET_4WAY_IF, &num_esc, 1);
}

#define ALARM_OK 0
#define ALARM_WARN 1
#define ALARM_ERROR 2
#define ALARM_CRIT 3

static void msp_send_alarms(struct msp_bridge *m) {
	union {
		uint8_t buf[0];
		struct {
			uint8_t state;
			char msg[MAX_ALARM_LEN];
		} __attribute__((packed)) alarm;
	} data;

	SystemAlarmsData alarm;
	SystemAlarmsGet(&alarm);

	// Special case early boot times -- just report boot reason
	if (PIOS_Thread_Systime() < BOOT_DISPLAY_TIME_MS) {
		data.alarm.state = ALARM_CRIT;
		const char *boot_reason = AlarmBootReason(alarm.RebootCause);
		strncpy((char*)data.alarm.msg, boot_reason, MAX_ALARM_LEN);
		data.alarm.msg[MAX_ALARM_LEN-1] = '\0';
		msp_send(m, MSP_ALARMS, data.buf, strlen((char*)data.alarm.msg)+1);
		return;
	}

	uint8_t state;
	data.alarm.state = ALARM_OK;
	int32_t len = AlarmString(&alarm, data.alarm.msg,
				  sizeof(data.alarm.msg), false, &state);
	switch (state) {
	case SYSTEMALARMS_ALARM_WARNING:
		data.alarm.state = ALARM_WARN;
		break;
	case SYSTEMALARMS_ALARM_ERROR:
		break;
	case SYSTEMALARMS_ALARM_CRITICAL:
		data.alarm.state = ALARM_CRIT;;
	}

	msp_send(m, MSP_ALARMS, data.buf, len+1);
}

static msp_state msp_state_checksum(struct msp_bridge *m, uint8_t b)
{
	if ((m->checksum ^ b) != 0) {
		return MSP_IDLE;
	}

	// Respond to interesting things.
	switch (m->cmd_id) {
	case MSP_API_VERSION:
		msp_send_api_version(m);
		break;
	case MSP_FC_VERSION:
		msp_send_fc_version(m);
		break;
	case MSP_FC_VARIANT:
		msp_send_fc_variant(m);
		break;
	case MSP_BOARD_INFO:
		msp_send_board_info(m);
		break;
	case MSP_BUILD_INFO2:
		msp_send_build_info_cf(m);
		break;
	case MSP_NAME:
		msp_send_name(m);
		break;
	case MSP_FEATURE:
		msp_send_feature(m);
		break;
	case MSP_CONFIG:
		msp_send_config(m);
		break;
	case MSP_BUILD_INFO:
		msp_send_build_info(m);
		break;
	case MSP_IDENT:
		msp_send_ident(m);
		break;
	case MSP_UID:
		msp_send_uid(m);
		break;
	case MSP_MOTOR:
		msp_send_motor(m);
		break;
	case MSP_RAW_GPS:
		msp_send_raw_gps(m);
		break;
	case MSP_COMP_GPS:
		msp_send_comp_gps(m);
		break;
	case MSP_ALTITUDE:
		msp_send_altitude(m);
		break;
	case MSP_ATTITUDE:
		msp_send_attitude(m);
		break;
	case MSP_STATUS:
		msp_send_status(m);
		break;
	case MSP_ANALOG:
		msp_send_analog(m);
		break;
	case MSP_RC:
		msp_send_channels(m);
		break;
	case MSP_RC_TUNING:
		msp_send_rc_tuning(m);
		break;
	case MSP_PID:
		msp_send_pid(m);
		break;
	case MSP_MISC:
		msp_send_misc(m);
		break;
	case MSP_BOXIDS:
		msp_send_boxids(m);
		break;
	case MSP_PIDNAMES:
		msp_send_pid_names(m);
		break;
	case MSP_SET_PID:
		msp_set_pid(m);
		break;
	case MSP_ALARMS:
		msp_send_alarms(m);
		break;
	case MSP_SET_4WAY_IF:
		msp_handle_4wif(m);
		break;
	default:
		msp_send_error(m, m->cmd_id);
		break;
	}
	return MSP_IDLE;
}

static msp_state msp_state_discard(struct msp_bridge *m, uint8_t b)
{
	return m->cmd_i++ == m->cmd_size ? MSP_IDLE : MSP_DISCARD;
}

/**
 * Process incoming bytes from an MSP query thing.
 * @param[in] b received byte
 * @return true if we should continue processing bytes
 */
static void msp_receive_byte(struct msp_bridge *m, uint8_t b)
{
	switch (m->state) {
	case MSP_IDLE:
		switch (b) {
		case '<': // uavtalk matching with 0x3c 0x2x 0xxx 0x0x
			m->state = MSP_MAYBE_UAVTALK2;
			break;
		case '$':
			m->state = MSP_HEADER_START;
			break;
		default:
			m->state = MSP_IDLE;
		}
		break;
	case MSP_HEADER_START:
		m->state = b == 'M' ? MSP_HEADER_M : MSP_IDLE;
		break;
	case MSP_HEADER_M:
		m->state = b == '<' ? MSP_HEADER_SIZE : MSP_IDLE;
		break;
	case MSP_HEADER_SIZE:
		m->state = msp_state_size(m, b);
		break;
	case MSP_HEADER_CMD:
		m->state = msp_state_cmd(m, b);
		break;
	case MSP_FILLBUF:
		m->state = msp_state_fill_buf(m, b);
		break;
	case MSP_CHECKSUM:
		m->state = msp_state_checksum(m, b);
		break;
	case MSP_DISCARD:
		m->state = msp_state_discard(m, b);
		break;
	case MSP_MAYBE_UAVTALK2:
		// e.g. 3c 20 1d 00
		// second possible uavtalk byte
		m->state = (b&0xf0) == 0x20 ? MSP_MAYBE_UAVTALK3 : MSP_IDLE;
		break;
	case MSP_MAYBE_UAVTALK3:
		// third possible uavtalk byte can be anything
		m->state = MSP_MAYBE_UAVTALK4;
		break;
	case MSP_MAYBE_UAVTALK4:
		m->state = MSP_IDLE;
		// If this looks like the fourth possible uavtalk byte, we're done
		if ((b & 0xf0) == 0) {
			PIOS_COM_TELEM_SER = m->com;

			m->handler = MSP_HANDLER_IDLE;
		}
		break;
	}
}

/**
 * Module start routine automatically called after initialization routine
 * @return 0 when was successful
 */
static int32_t uavoMSPBridgeStart(void)
{
	if (!module_enabled) {
		// give port to telemetry if it doesn't have one
		// stops board getting stuck in condition where it can't be connected to gcs
		if(!PIOS_COM_TELEM_SER)
			PIOS_COM_TELEM_SER = pios_com_msp_id;

		return -1;
	}

	struct pios_thread *task = PIOS_Thread_Create(
		uavoMSPBridgeTask, "uavoMSPBridge",
		STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
	TaskMonitorAdd(TASKINFO_RUNNING_UAVOMSPBRIDGE,
			task);

	return 0;
}

static void setMSPSpeed(struct msp_bridge *m)
{
	if (m->com) {
		uint8_t speed;
		ModuleSettingsMSPSpeedGet(&speed);

		PIOS_HAL_ConfigureSerialSpeed(m->com, speed);
	}
}

/**
 * Module initialization routine
 * @return 0 when initialization was successful
 */
static int32_t uavoMSPBridgeInitialize(void)
{
	if (pios_com_msp_id && PIOS_Modules_IsEnabled(PIOS_MODULE_UAVOMSPBRIDGE)) {
		msp = PIOS_malloc(sizeof(*msp));
		if (msp != NULL) {
			memset(msp, 0x00, sizeof(*msp));

			msp->com = pios_com_msp_id;

			module_enabled = true;
			return 0;
		}
	}

	return -1;
}
MODULE_INITCALL(uavoMSPBridgeInitialize, uavoMSPBridgeStart)

void esc4wayProcess(void *mspPort);

/**
 * Main task routine
 * @param[in] parameters parameter given by PIOS_Thread_Create()
 */
static void uavoMSPBridgeTask(void *parameters)
{
	setMSPSpeed(msp);

	while (1) {
		switch (msp->handler) {
		case MSP_HANDLER_MSP:
			(void) 0;

			uint8_t b = 0;

			uint16_t count =
				PIOS_COM_ReceiveBuffer(msp->com, &b, 1, 3000);

			if (count) {
				msp_receive_byte(msp, b);
			}

			break;

#ifdef PIOS_INCLUDE_4WAY
		case MSP_HANDLER_4WIF:
			esc4wayProcess(&msp->com);
			msp->handler = MSP_HANDLER_MSP;

			PIOS_Thread_Sleep(3);
			actuator_interlock = ACTUATOR_INTERLOCK_OK;
			break;
#endif

		case MSP_HANDLER_IDLE:
			// Returning is considered risky here as
			// that's unusual and this is an edge case.
			while (1) {
				PIOS_Thread_Sleep(60*1000);
			}
			break;

		default:
			PIOS_Assert(0);
		}
	}
}

#endif //PIOS_INCLUDE_MSP_BRIDGE
/**
 * @}
 * @}
 */
