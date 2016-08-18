#define FPPROTO_MAX_SERVOS 16
#define FPPROTO_MAX_BANKS 4
#define FPPROTO_MAX_RCCHANS 10
#define FPPROTO_MAX_ADCCHANS 4

/* 34 bytes */

#define FLYINGPICMD_ACTUATOR 0xfc
#define FLYINGPICMD_CFG 0xfa
#define FLYINGPIRESP_IO 0x10

struct flyingpicmd_actuator_fc {
	/* fractional values; 0 = min, 0xffff = max */
	uint16_t values[FPPROTO_MAX_SERVOS];

	uint16_t led_status : 1;
	uint16_t flags_resv : 15;
} __attribute__((__packed__));

struct flyingpicmd_cfg_fa {
	/* This here is simpler than the controller-side configuration.
	 * min is expected to be a true minimum; max is a true maximum.
	 * The values that come in are scaled linearly between min and max
	 * using fixed point math
	 */
	struct {
		uint16_t min;	/* microseconds */
		uint16_t max;
	} actuators[FPPROTO_MAX_SERVOS];

	/* Rate in Hz;  0 is oneshot */
	uint16_t rate[FPPROTO_MAX_BANKS];
	uint16_t receiver_protocol;	/* If less than 0x100, this is from
					 * HwShared.PortTypes
					 */
	uint16_t flags_resv;	
} __attribute__((__packed__));

/* all IO data stuff must be equal/less than length of flyingpi_actuator_fc */
/* 20 + 8 + 2 + 2 = 32 bytes */
struct flyingpiresp_io_10 {
	uint16_t chan_data[FPPROTO_MAX_RCCHANS];
	uint16_t adc_data[FPPROTO_MAX_ADCCHANS];

	uint16_t valid_messages_recvd;
	uint16_t flags_resv;
} __attribute__((__packed__));

/* 3 bytes header */
struct flyingpi_msg {
	uint8_t crc8;
	uint8_t id;

	union {
		struct flyingpicmd_actuator_fc actuator_fc;
		struct flyingpicmd_cfg_fa cfg_fa;
		struct flyingpiresp_io_10 io_10;
	} body;
} __attribute__((__packed__));

static inline bool flyingpi_calc_crc(struct flyingpi_msg *msg, bool fill_in,
		int *total_len) {
	uint8_t len;

	switch (msg->id) {
		case FLYINGPICMD_ACTUATOR:
			len = sizeof(msg->body.actuator_fc);
			break;
		case FLYINGPICMD_CFG:
			len = sizeof(msg->body.cfg_fa);
			break;
		case FLYINGPIRESP_IO:
			len = sizeof(msg->body.io_10);
			break;
		default:
			return false;
	}

	/* Fixup to include ID */
	uint8_t crc8 = PIOS_CRC_updateCRC(0, ((uint8_t *) &msg->body) - 1,
			len+1);

	if (fill_in) {
		msg->crc8 = crc8;
	}

	if (total_len) {
		*total_len = len + 2;
	}

	return msg->crc8 == crc8;
}
