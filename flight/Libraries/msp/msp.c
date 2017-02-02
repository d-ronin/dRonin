/**
 ******************************************************************************
 * @file       msp.c
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

#include "msp.h"

#define MSP_OVERHEAD_BYTES (2 /* preamble */ \
                          + 1 /* direction */ \
                          + 1 /* size */ \
                          + 1 /* command */ \
                          + 1 /* checksum */)

#define MSP_PARSER_MAGIC 0x32aedc07 

enum msp_state {
	MSP_STATE_IDLE,
	MSP_STATE_PREAMBLE,
	MSP_STATE_DIRECTION,
	MSP_STATE_SIZE,
	MSP_STATE_COMMAND,
	MSP_STATE_DATA,
	MSP_STATE_CHECKSUM,
	MSP_STATE_DISCARD,
};

struct msp_parser {
	uint32_t magic;

	msp_handler_t handler;
	void *handler_context;

	enum msp_state state;
	enum msp_parser_type type;

	uint8_t data_len;
	uint8_t command;
	uint8_t checksum;
	uint8_t data_buf[255];
	uint8_t data_rcvd;
};

static bool parser_validate(struct msp_parser *p)
{
	return p != NULL && p->magic == MSP_PARSER_MAGIC;
}

static bool call_handler(struct msp_parser *p, enum msp_message_id id, void *buf, uint8_t len)
{
	if (!p->handler)
		return false;
	return p->handler(id, buf, len, p->handler_context);
}

static enum msp_state msp_parse_idle(struct msp_parser *p, uint8_t b)
{
	if (b == '$')
		return MSP_STATE_PREAMBLE;
	return MSP_STATE_IDLE;
}

static enum msp_state msp_parse_preamble(struct msp_parser *p, uint8_t b)
{
	if (b == 'M')
		return MSP_STATE_DIRECTION;
	return MSP_STATE_IDLE;
}

static enum msp_state msp_parse_direction(struct msp_parser *p, uint8_t b)
{
	switch (b) {
	case '>':
		if (p->type == MSP_PARSER_CLIENT)
			return MSP_STATE_SIZE;
		break;
	case '<':
		if (p->type == MSP_PARSER_SERVER)
			return MSP_STATE_SIZE;
		break;
	default:
		break;
	}
	return MSP_STATE_IDLE;
}

static enum msp_state msp_parse_size(struct msp_parser *p, uint8_t b)
{
	p->checksum = b;
	p->data_len = b;
	p->data_rcvd = 0;
	return MSP_STATE_COMMAND;
}

static enum msp_state msp_parse_command(struct msp_parser *p, uint8_t b)
{
	p->checksum ^= b;
	p->command = b;
	if (p->data_len) {
		if (p->data_len <= NELEMENTS(p->data_buf))
			return MSP_STATE_DATA;
		else
			return MSP_STATE_DISCARD;
	} else {
		return MSP_STATE_CHECKSUM;
	}
}

static enum msp_state msp_parse_data(struct msp_parser *p, uint8_t b)
{
	p->checksum ^= b;
	p->data_buf[p->data_rcvd++] = b;
	if (p->data_rcvd < p->data_len)
		return MSP_STATE_DATA;
	else
		return MSP_STATE_CHECKSUM;
}

static enum msp_state msp_parse_checksum(struct msp_parser *p, uint8_t b)
{
	p->checksum ^= b;
	if (!p->checksum)
		call_handler(p, p->command, p->data_rcvd ? p->data_buf : NULL, p->data_rcvd);

	return MSP_STATE_IDLE;
}

static enum msp_state msp_parse_discard(struct msp_parser *p, uint8_t b)
{
	p->data_rcvd++;
	/* consume checksum byte too */
	if (p->data_rcvd <= p->data_len)
		return MSP_STATE_DISCARD;
	else
		return MSP_STATE_IDLE;
}

static void process_byte(struct msp_parser *p, uint8_t b)
{
	switch (p->state) {
	case MSP_STATE_IDLE:
		p->state = msp_parse_idle(p, b);
		break;
	case MSP_STATE_PREAMBLE:
		p->state = msp_parse_preamble(p, b);
		break;
	case MSP_STATE_DIRECTION:
		p->state = msp_parse_direction(p, b);
		break;
	case MSP_STATE_SIZE:
		p->state = msp_parse_size(p, b);
		break;
	case MSP_STATE_COMMAND:
		p->state = msp_parse_command(p, b);
		break;
	case MSP_STATE_DATA:
		p->state = msp_parse_data(p, b);
		break;
	case MSP_STATE_CHECKSUM:
		p->state = msp_parse_checksum(p, b);
		break;
	case MSP_STATE_DISCARD:
		p->state = msp_parse_discard(p, b);
		break;
	}
}

/* public */

struct msp_parser *msp_parser_init(enum msp_parser_type type)
{
	struct msp_parser *p = PIOS_malloc(sizeof(*p));
	if (!p)
		return NULL;

	memset(p, 0, sizeof(*p));

	p->state = MSP_STATE_IDLE;
	p->type = type;
	p->magic = MSP_PARSER_MAGIC;

	return p;
}

int32_t msp_process_buffer(struct msp_parser *parser, void *buf, uint8_t len)
{
	if (!parser_validate(parser))
		return -1;

	for (unsigned i = 0; i < len; i++)
		process_byte(parser, ((uint8_t *)buf)[i]);

	return len;
}

int32_t msp_process_com(struct msp_parser *parser, struct pios_com_dev *com)
{
	if (!parser_validate(parser))
		return -1;

	int32_t len = 0;
	uint8_t b;
	/* TODO: fix PIOS_COM to take a pointer */
	while (PIOS_COM_ReceiveBuffer((uintptr_t)com, &b, 1, 0)) {
		process_byte(parser, b);
		len++;
	}

	return len;
}

int32_t msp_send_com(struct msp_parser *parser, struct pios_com_dev *com, enum msp_message_id msg_id, void *payload, uint8_t len)
{
	if (!parser_validate(parser))
		return -1;

	uint8_t dir = (parser->type == MSP_PARSER_SERVER) ? '>' : '<';
	uint8_t hdr[] = {'$', 'M', dir, len, msg_id};
	/* TODO: not assume success */
	/* TODO: fix PIOS_COM to take a pointer */
	int32_t written = PIOS_COM_SendBuffer((uintptr_t)com, hdr, NELEMENTS(hdr));
	if (len)
		written += PIOS_COM_SendBuffer((uintptr_t)com, payload, len);
	uint8_t checksum = len ^ (uint8_t)msg_id;
	for (unsigned i = 0; i < len; i++)
		checksum ^= ((uint8_t *)payload)[i];
	written += PIOS_COM_SendBuffer((uintptr_t)com, &checksum, 1);

	return written;
}

int32_t msp_register_handler(struct msp_parser *parser, msp_handler_t handler, void *context)
{
	if (!parser_validate(parser))
		return -1;

	parser->handler = handler;
	parser->handler_context = context;

	return 0;
}

/**
 * @}
 * @}
 */
