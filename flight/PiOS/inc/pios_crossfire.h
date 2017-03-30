/**
 ******************************************************************************
 * @file       pios_crossfire.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_Crossfire PiOS TBS Crossfire receiver driver
 * @{
 * @brief Receives and decodes CRSF protocol receiver packets
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

#include "pios.h"

#ifndef PIOS_CROSSFIRE_H
#define PIOS_CROSSFIRE_H

#include "pios_rcvr.h"

// There's 16 channels in the RC channel payload, the Crossfire currently
// only uses 12. Asked Perna a while ago to always send RSSI and LQ on
// 15 and 16, said was nice idea, but never showed up in change logs.
#define PIOS_CROSSFIRE_CHANNELS		16

// Lengths of the type and CRC fields.
#define CRSF_ADDRESS_LEN			1
#define CRSF_LENGTH_LEN				1
#define CRSF_CRC_LEN				1
#define CRSF_TYPE_LEN				1

// Maximum payload in the protocol can be 32 bytes.
// Should figure out whether that includes type and CRC, or not.
#define CRSF_MAX_PAYLOAD			32

// Frame types.
#define CRSF_FRAME_GPS				0x02
#define CRSF_FRAME_BATTERY			0x08
#define CRSF_FRAME_RCCHANNELS		0x16
#define CRSF_FRAME_ATTITUDE			0x1e

// Payload sizes
#define CRSF_PAYLOAD_GPS			15
#define CRSF_PAYLOAD_BATTERY		8
#define CRSF_PAYLOAD_RCCHANNELS		22
#define CRSF_PAYLOAD_ATTITUDE		6
// Maximum payload in the protocol can be 32 bytes.
// Should figure out whether that includes type and CRC, or not.
#define CRSF_MAX_PAYLOAD			32


#define CRSF_TIMING_MAXFRAME		1000
#define CRSF_TIMING_FRAMEDISTANCE	4000

// We don't need those. Yet. More like a reference right now.
struct crsf_payload_gps {

	int32_t latitude, longitude;
	uint16_t groundspeed;
	uint16_t heading;
	uint16_t altitude;
	uint8_t num_satellites;

} __attribute__((packed));

struct crsf_payload_battery {

	uint16_t voltage;
	uint16_t current;
	uint8_t capacity[3];
	uint8_t batt_remaining;

} __attribute__((packed));

struct crsf_payload_attitude {

	int16_t pitch;
	int16_t roll;
	int16_t yaw;

} __attribute__((packed));

union crsf_combo_payload {
	struct crsf_payload_gps gps;
	struct crsf_payload_battery battery;
	struct crsf_payload_attitude attitude;
	uint8_t payload[CRSF_MAX_PAYLOAD + CRSF_CRC_LEN];
};

struct crsf_frame_t {
	// Module address. Not sure what's the point, assuming provision for
	// a bus type deal. Not used.
	uint8_t	dev_addr;

	// Length of the payload following this field. Length includes CRC and
	// type field in addition to payload.
	uint8_t	length;

	// Frame type
	uint8_t type;

	// The payload plus CRC appendix.
	uint8_t payload[CRSF_MAX_PAYLOAD + CRSF_CRC_LEN];
} __attribute__((packed));

// Macro referring to the maximum frame size.
#define CRSF_MAX_FRAMELEN		sizeof(struct crsf_frame_t)

// Get formal payload length.
#define CRSF_PAYLOAD_LEN(x)		(CRSF_TYPE_LEN+(x)+CRSF_CRC_LEN)

extern const struct pios_rcvr_driver pios_crossfire_rcvr_driver;

/**
 * @brief Initialises the TBS Crossfire Rx driver with a serial port
 * @param[out] crsf_id Crossfire receiver device handle
 * @param[in] driver PiOS COM driver for the serial port
 * @param[in] uart_id UART port driver handle
 * @retval 0 on success, otherwise error
 */
int PIOS_Crossfire_Init(uintptr_t *crsf_id, const struct pios_com_driver *driver,
		uintptr_t uart_id);

int PIOS_Crossfire_InitTelemetry(uintptr_t crsf_id);

int PIOS_Crossfire_SendTelemetry(uintptr_t crsf_id, uint8_t *buf, uint8_t bytes);

bool PIOS_Crossfire_IsFailsafed();

#endif // PIOS_CROSSFIRE_H

  /**
  * @}
  * @}
  */
