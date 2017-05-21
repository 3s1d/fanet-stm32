/*
 * serial_fanet.h
 *
 *  Created on: May 21, 2017
 *      Author: sid
 */

#ifndef COM_CMD_FANET_H_
#define COM_CMD_FANET_H_

/* FANET */
#define CMD_FANET_STATE			'S'
#define CMD_FANET_TRANSMIT		'T'
#define CMD_FANET_ADDR			'A'
#define CMD_FANET_CONFIG		'C'

/* Fanet Replies */
#define FANET_CMD_OK			(FANET_CMD_START "R OK")
#define FANET_CMD_ERROR			(FANET_CMD_START "R ERR")
#define FANET_CMD_MSG			(FANET_CMD_START "R MSG")
#define FANET_CMD_ACK			(FANET_CMD_START "R ACK")
#define FANET_CMD_NACK			(FANET_CMD_START "R NACK")


/* range 1-59 */
#define FN_REPLY_OK			FANET_CMD_OK, 	 0,  NULL
#define FN_REPLYM_INITIALIZED		FANET_CMD_MSG,   1,  "initialized"
#define FN_REPLYE_RADIO_FAILED		FANET_CMD_ERROR, 2,  "radio failed"
#define FN_REPLYE_UNKNOWN_CMD		FANET_CMD_ERROR, 5,  "unknown command"
#define FN_REPLYE_FN_UNKNOWN_CMD	FANET_CMD_ERROR, 6,  "unknown FN command"
#define FN_REPLYE_NO_SRC_ADDR		FANET_CMD_ERROR, 10, "no source address"
#define FN_REPLYE_INVALID_ADDR		FANET_CMD_ERROR, 11, "invalid address"
#define FN_REPLYE_INCOMPATIBLE_TYPE	FANET_CMD_ERROR, 12, "incompatible type"
#define FN_REPLYE_TX_BUFF_FULL		FANET_CMD_ERROR, 14, "tx buffer full"
#define FN_REPLYE_CMD_TOO_SHORT		FANET_CMD_ERROR, 30, "too short"

void cmd_fanet_mux(char *cmd);

#endif /* COM_CMD_FANET_H_ */
