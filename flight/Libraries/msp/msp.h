/**
 ******************************************************************************
 * @file       msp.h
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

#ifndef MSP_H_
#define MSP_H_

#include "pios.h"
#include "msp_messages.h"

/** Opaque structure for handle to parser instances */
struct msp_parser;

/** 
 * @brief Handler to be called when valid MSP messages are recieved
 * @param[in] msg_id The MSP command/id of the message
 * @param[in] data Buffer containing the message payload (or NULL if none)
 * @param[in] len Length of data in buffer
 * @param[in] context Context pointer provided when registering handler
 * @retval true Message was handled successfully (future use/multi-handler for incompatible protocol extensions)
 * @retval false Message was not handled successfully (in future, may cause message to be passed to another handler)
 */
typedef bool (*msp_handler_t)(enum msp_message_id msg_id, void *data, uint8_t len, void *context);

/** Whether the parser should act as a client or server */
enum msp_parser_type {
	MSP_PARSER_SERVER, /**< Act as a server (e.g. flight controller) */
	MSP_PARSER_CLIENT  /**< Act as a client (e.g. OSD) */
};

/** 
 * @brief Initialize a new parser instance
 * @param[in] type Whether to act as client or server
 * @return Handle to struct msp_parser on success, NULL on failure
 */
struct msp_parser *msp_parser_init(enum msp_parser_type type);
/** 
 * @brief Process MSP stream from buffer
 * @param[in] parser Handle to parser instance
 * @param[in] buf Buffer containing received stream
 * @param[in] len Length of buffer
 * @return Number of bytes read from buffer (always all of them), or -1 on failure
 */
int32_t msp_process_buffer(struct msp_parser *parser, void *buf, uint8_t len);
/** 
 * @brief Process MSP stream from PIOS_COM
 * @param[in] parser Handle to parser instance
 * @param[in] com Handle to PIOS_COM instance
 * @return Number of bytes read from com (all available), or -1 on failure
 */
int32_t msp_process_com(struct msp_parser *parser, struct pios_com_dev *com);
/** 
 * @brief Construct and send an MSP message via PIOS_COM
 * @param[in] parser Handle to parser instance
 * @param[in] com Handle to PIOS_COM instance
 * @param[in] msg_id Message ID/Command
 * @param[in] payload Buffer containing payload, or NULL if none
 * @param[in] len Length of payload (bytes)
 * @return Number of bytes read from com (all available), or -1 on failure
 */
int32_t msp_send_com(struct msp_parser *parser, struct pios_com_dev *com, enum msp_message_id msg_id, void *payload, uint8_t len);
/** 
 * @brief Register a handler for valid received messages
 * @param[in] parser Handle to parser instance
 * @param[in] handler Handler function
 * @param[in] context Context pointer that will be passed into handler
 * @retval -1 Invalid parser instance
 * @retval 0 Success
 */
int32_t msp_register_handler(struct msp_parser *parser, msp_handler_t handler, void *context);

#endif // MSP_H_

/**
 * @}
 * @}
 */
