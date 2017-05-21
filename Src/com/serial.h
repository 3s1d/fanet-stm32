/*
 * serial.h
 *
 *  Created on: May 20, 2017
 *      Author: sid
 */

#ifndef COM_SERIAL_H_
#define COM_SERIAL_H_

#include <stdbool.h>
#include "stm32l4xx.h"
#include "stm32l4xx_hal_uart.h"

#define FANET_CMD_START			"#FN"
#define BT_CMD_START			"#BT"
#define DONGLE_CMD_START		"#DG"

#define SEPARATOR			','

/* Dongle Replies */
#define DONGLE_CMD_OK			(DONGLE_CMD_START "R OK")
#define DONGLE_CMD_MSG			(DONGLE_CMD_START "R MSG")
#define DONGLE_CMD_ERROR		(DONGLE_CMD_START "R ERR")



/* values <= 0 removes code and message */

/* range 60-99 */
#define DN_REPLY_OK			DONGLE_CMD_OK, 	 0,  NULL
#define DN_REPLYE_DONGLE_UNKNOWN_CMD	DONGLE_CMD_ERROR,60, "unknown DG command"
#define DN_REPLYE_JUMP			DONGLE_CMD_ERROR,61, "unknown jump point"
#define DN_REPLYE_POWER			DONGLE_CMD_ERROR,70, "power switch failed"
#define DN_REPLYE_TOOLESSPARAMETER	DONGLE_CMD_ERROR,80, "too less parameter"
#define DN_REPLYE_UNKNOWNPARAMETER	DONGLE_CMD_ERROR,81, "unknown parameter"

/*
 * Normal Commands
 * State: 		#FNS lat(deg),lon(deg),alt(m),speed(km/h),climb(m/s),heading(deg)[,turn(deg/s)]		note: all values in float (NOT hex)
 * Address: 		#FNA manufacturer(hex),id(hex)
 * Config: 		#FNC type(0..7),onlinelogging(0..1)							note: type see protocol.txt
 * Transmit: 		#FNT type,dest_manufacturer,dest_id,forward,ack_required,length,length*2hex		note: all values in hex
 *
 * Receive a Frame:	#FNF src_manufacturer,src_id,broadcast,signature,type,payloadlength,payload
 *
 * Maintenance
 * Version:		#DGV
 * Standby:		#DGP powermode(0..1)									note: w/o status is returned
 * Region:		#DGL freq(868,915),power(2..20 (dBm))
//todo: txpower, channel
//todo
 * Jump to DFU:		#DGJ BLsw										note: not working probably for USB
 * 			#DGJ BLhw										note: requires external RC network
 * 														recommended: use 2 GPIOs connected
 //
 * 														to RESEST and BOOT0
 *
 * Bluetooth Commands
 * Name:		#BTN name
 * Address:		#BTA addr										note: cmd may be disabled
 */


void serial_init(UART_HandleTypeDef *uart);

bool serial_poll(void);

void serial_reply(char *type, int key, char *msg);

#endif /* COM_SERIAL_H_ */
