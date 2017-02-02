/**
 ******************************************************************************
 * @file       msp_messages.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup Libraries
 * @{
 * @addtogroup MSP Protocol Library
 * @{
 * @brief Deals with packing/unpacking and MSP streams
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

#ifndef MSP_MESSAGES_H_
#define MSP_MESSAGES_H_

/**
 * All MSP message types supported
 * MSP_MSG_ and msp_ prefixes are used for standard MSP protocol,
 * as documented at http://www.multiwii.com/wiki/index.php?title=Multiwii_Serial_Protocol
 * All naming follows this standard where possible/appropriate
 *
 * MSP_EXT_ and mspext_ prefixes are used for non-standard protocol extensions
 */
enum msp_message_id {
	/* read */
	MSP_MSG_IDENT                  = 100, /* 0x64 */
	MSP_MSG_STATUS                 = 101, /* 0x65 */
	MSP_MSG_RAW_IMU                = 102, /* 0x66 */
	MSP_MSG_SERVO                  = 103, /* 0x67 */
	MSP_MSG_MOTOR                  = 104, /* 0x68 */
	MSP_MSG_RC                     = 105, /* 0x69 */
	MSP_MSG_RAW_GPS                = 106, /* 0x6a */
	MSP_MSG_COMP_GPS               = 107, /* 0x6b */
	MSP_MSG_ATTITUDE               = 108, /* 0x6c */
	MSP_MSG_ALTITUDE               = 109, /* 0x6d */
	MSP_MSG_ANALOG                 = 110, /* 0x6e */
	MSP_MSG_RC_TUNING              = 111, /* 0x6f */
	MSP_MSG_PID                    = 112, /* 0x70 */
	MSP_MSG_BOX                    = 113, /* 0x71 */
	MSP_MSG_MISC                   = 114, /* 0x72 */
	MSP_MSG_MOTOR_PINS             = 115, /* 0x73 */
	MSP_MSG_BOXNAMES               = 116, /* 0x74 */
	MSP_MSG_PIDNAMES               = 117, /* 0x75 */
	MSP_MSG_WP                     = 118, /* 0x76 */
	MSP_MSG_BOXIDS                 = 119, /* 0x77 */
	MSP_MSG_SERVO_CONF             = 120, /* 0x78 */
	/* write */
	MSP_MSG_SET_RAW_RC             = 200, /* 0xc8 */
	MSP_MSG_SET_RAW_GPS            = 201, /* 0xc9 */
	MSP_MSG_SET_PID                = 202, /* 0xca */
	MSP_MSG_SET_BOX                = 203, /* 0xcb */
	MSP_MSG_SET_RC_TUNING          = 204, /* 0xcc */
	MSP_MSG_ACC_CALIBRATION        = 205, /* 0xcd */
	MSP_MSG_MAG_CALIBRATION        = 206, /* 0xce */
	MSP_MSG_SET_MISC               = 207, /* 0xcf */
	MSP_MSG_RESET_CONF             = 208, /* 0xd0 */
	MSP_MSG_SET_WP                 = 209, /* 0xd1 */
	MSP_MSG_SELECT_SETTING         = 210, /* 0xd2 */
	MSP_MSG_SET_HEAD               = 211, /* 0xd3 */
	MSP_MSG_SET_SERVO_CONF         = 212, /* 0xd4 */
	MSP_MSG_SET_MOTOR              = 214, /* 0xd6 */
	MSP_MSG_BIND                   = 240, /* 0xf0 */
	MSP_MSG_EEPROM_WRITE           = 250, /* 0xfa */

	/* unofficial protocol extensions from *flight,
	 * here be dragons (and some outright silliness) 
	 * Note: please don't add extensions marked as private,
	 * and preferably not commands that provide a subset of
	 * the original Multiwii protocol (use the original ones
	 * for wider compatibility) */
	MSP_EXT_NAV_STATUS             = 121, /* 0x79 */
	MSP_EXT_NAV_CONFIG             = 122, /* 0x7a */
	MSP_EXT_FW_CONFIG              = 123, /* 0x7b */
	MSP_EXT_UID                    = 160, /* 0xa0 */
	MSP_EXT_GPSSVINFO              = 164, /* 0xa4 */
	MSP_EXT_GPSDEBUGINFO           = 166, /* 0xa6 */
	MSP_EXT_ACC_TRIM               = 240, /* 0xf0  WARNING: shadows MSP_MSG_BIND */
	MSP_EXT_SERVOMIX_CONF          = 241, /* 0xf1 */

	MSP_EXT_SET_NAV_CONFIG         = 215, /* 0xd7 */
	MSP_EXT_SET_FW_CONFIG          = 216, /* 0xd8 */
	MSP_EXT_SET_ACC_TRIM           = 239, /* 0xef */
	MSP_EXT_SET_SERVOMIX_CONF      = 242, /* 0xf2 */

	MSP_EXT_DEBUGMSG               = 253, /* 0xfd */
	MSP_EXT_DEBUG                  = 254, /* 0xfe */
};

struct msp_ident {
	uint8_t version;
	uint8_t multitype;
	uint8_t msp_version;
	uint32_t capability;
} __attribute((packed));

struct msp_status {
	uint16_t cycle_time; /* us */
	uint16_t i2c_errors_count;
	uint16_t sensor;
	uint32_t flag;
	uint8_t current_set;
} __attribute((packed));

/* this is a bit of a mess, unit depends on board sensors :/ */
struct msp_raw_imu {
	int16_t accx;
	int16_t accy;
	int16_t accz;
	int16_t gyrx;
	int16_t gyry;
	int16_t gyrz;
	int16_t magx;
	int16_t magy;
	int16_t magz;
} __attribute((packed));

/* Note: MSP doc is ambiguous on the length of these */
struct msp_servo {
	uint16_t servo[8]; /* [1000:2000] us */
} __attribute((packed));

struct msp_motor {
	uint16_t motor[8];
} __attribute((packed)); /* [1000:2000] us */

struct msp_set_motor {
	uint16_t motor[8];
} __attribute((packed)); /* [1000:2000] us */

struct msp_rc {
	uint16_t rc_data[8];
} __attribute((packed)); /* [1000:2000] us */

struct msp_set_raw_rc {
	uint16_t rc_data[8];
} __attribute((packed)); /* [1000:2000] us */

struct msp_raw_gps {
	uint8_t fix; /* bool */
	uint8_t num_sat;
	uint32_t coord_lat; /* 1 / 10 000 000 deg */
	uint32_t coord_lon; /* 1 / 10 000 000 deg */
	uint16_t altitude; /* m */
	uint16_t speed; /* cm/s */
	uint16_t ground_course; /* deg * 10 */
} __attribute((packed));

/* inexplicably no ground_course in this one? */
struct msp_set_raw_gps {
	uint8_t fix; /* bool */
	uint8_t num_sat;
	uint32_t coord_lat; /* 1 / 10 000 000 deg */
	uint32_t coord_lon; /* 1 / 10 000 000 deg */
	uint16_t altitude; /* m */
	uint16_t speed; /* cm/s */
} __attribute((packed));

struct msp_comp_gps {
	uint16_t distance_to_home;
	uint16_t direction_to_home; /* [-180:180] deg */
	uint8_t update;
} __attribute((packed));

struct msp_attitude {
	int16_t angx; /* [-1800:1800] 1/10 deg */
	int16_t angy; /* [-900:900] 1/10 deg */
	int16_t heading; /* [-180:180] deg */
} __attribute((packed));

struct msp_altitude {
	int32_t est_alt; /* cm */
	int16_t vario; /* cm/s */
} __attribute((packed));

struct msp_analog {
	uint8_t vbat; /* 1/10 V */
	uint16_t power_meter_sum;
	uint16_t rssi; /* [0:1023] */
	uint16_t amperage;
} __attribute((packed));

struct msp_rc_tuning {
	uint8_t rc_rate; /* [0:100] */
	uint8_t rc_expo; /* [0:100] */
	uint8_t roll_pitch_rate; /* [0:100] */
	uint8_t yaw_rate; /* [0:100] */
	uint8_t dyn_thr_pid; /* [0:100] */
	uint8_t throttle_mid; /* [0:100] */
	uint8_t throttle_expo; /* [0:100] */
} __attribute((packed));

struct msp_set_rc_tuning {
	uint8_t rc_rate;
	uint8_t rc_expo;
	uint8_t roll_pitch_rate;
	uint8_t yaw_rate;
	uint8_t dyn_thr_pid;
	uint8_t throttle_mid;
	uint8_t throttle_expo;
} __attribute((packed));

struct _msp_pid_item {
	uint8_t p;
	uint8_t i;
	uint8_t d;
} __attribute((packed));

struct msp_pid {
	struct _msp_pid_item roll;
	struct _msp_pid_item pitch;
	struct _msp_pid_item yaw;
	struct _msp_pid_item alt;
	struct _msp_pid_item pos;
	struct _msp_pid_item posr;
	struct _msp_pid_item navr;
	struct _msp_pid_item level;
	struct _msp_pid_item mag;
	struct _msp_pid_item vel;
} __attribute((packed));

struct msp_set_pid {
	struct _msp_pid_item roll;
	struct _msp_pid_item pitch;
	struct _msp_pid_item yaw;
	struct _msp_pid_item alt;
	struct _msp_pid_item pos;
	struct _msp_pid_item posr;
	struct _msp_pid_item navr;
	struct _msp_pid_item level;
	struct _msp_pid_item mag;
	struct _msp_pid_item vel;
} __attribute((packed));

/* TODO: this is extremely poorly specified */
struct msp_box {
	uint16_t boxitems[1];
} __attribute__((packed));

struct msp_set_box {
	uint16_t boxitems[1];
} __attribute__((packed));

struct msp_misc {
	uint16_t power_trigger;
	uint16_t min_throttle; /* [1000:2000] us, neutral value */
	uint16_t max_throttle; /* [1000:2000] us, max value */
	uint16_t min_command; /* [1000:2000] us, min value */
	uint16_t failsafe_throttle; /* [1000:2000] us */
	uint16_t arm_count;
	uint32_t lifetime;
	uint16_t mag_declination; /* 1/10 deg */
	uint8_t vbat_scale;
	uint8_t vbat_warn1; /* 1/10 V */
	uint8_t vbat_warn2; /* 1/10 V */
	uint8_t vbat_crit; /* 1/10 V */
} __attribute__((packed));

struct msp_set_misc {
	uint16_t power_trigger;
	uint16_t min_throttle; /* [1000:2000] us, neutral value */
	uint16_t max_throttle; /* [1000:2000] us, max value */
	uint16_t min_command; /* [1000:2000] us, min value */
	uint16_t failsafe_throttle; /* [1000:2000] us */
	uint16_t arm_count;
	uint32_t lifetime;
	uint16_t mag_declination; /* 1/10 deg */
	uint8_t vbat_scale;
	uint8_t vbat_warn1; /* 1/10 V */
	uint8_t vbat_warn2; /* 1/10 V */
	uint8_t vbat_crit; /* 1/10 V */
} __attribute__((packed));

struct msp_motor_pins {
	uint8_t pwm_pin[8];
} __attribute__((packed));

/* cannot know length in advance */
struct msp_boxnames {
	char items[0]; /* ; separated */
} __attribute__((packed));

/* cannot know length in advance */
struct msp_pidnames {
	char items[0]; /* ; separated */
} __attribute__((packed));

struct msp_wp {
	uint8_t wp_no; /* home = 0, hold = 15 */
	uint32_t lat;
	uint32_t lon;
	uint32_t alt_hold;
	uint16_t heading;
	uint16_t time_to_stay;
	uint8_t nav_flag;
} __attribute__((packed));

struct msp_set_wp {
	uint8_t wp_no; /* home = 0, hold = 15 */
	uint32_t lat;
	uint32_t lon;
	uint32_t alt_hold;
	uint16_t heading;
	uint16_t time_to_stay;
	uint8_t nav_flag;
} __attribute__((packed));

struct msp_boxids {
	uint8_t checkbox_items[0]; /* TODO: poor specification */
} __attribute__((packed));

/* NOTE: cleanflight made some incompatible extensions to this *sigh* */
struct _msp_servo_conf_item {
	uint16_t min; /* [1000:2000] us */
	uint16_t max; /* [1000:2000] us */
	uint16_t middle; /* [1000:2000] us */
	uint8_t rate; /* [0:100] ?? */
} __attribute__((packed));

struct msp_servo_conf {
	struct _msp_servo_conf_item servos[8];
} __attribute__((packed));

struct msp_select_setting {
	uint8_t current_set;
} __attribute__((packed));

struct msp_set_head {
	uint16_t mag_hold;
} __attribute__((packed));

#endif // MSP_MESSAGES_H_

/**
 * @}
 * @}
 */
