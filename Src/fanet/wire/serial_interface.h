/*
 * serial.h
 *
 *  Created on: 1 Oct 2016
 *      Author: sid
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include <string.h>
#include "serial.h"

#include "../fanet.h"
#include "../radio/fmac.h"


#define FANET_CMD_START			"#FN"
#define BT_CMD_START			"#BT"
#define DONGLE_CMD_START		"#DG"
#ifdef FLARM
#define FLARM_CMD_START			"#FA"
#endif
#define SEPARATOR			','


/* Fanet Replies */
#define FANET_CMD_OK			FANET_CMD_START "R OK"
#define FANET_CMD_ERROR			FANET_CMD_START "R ERR"
#define FANET_CMD_MSG			FANET_CMD_START "R MSG"
#define FANET_CMD_ACK			FANET_CMD_START "R ACK"
#define FANET_CMD_NACK			FANET_CMD_START "R NACK"

/* Bluetooth Replies */
#define BT_CMD_OK			BT_CMD_START "R OK"
#define BT_CMD_WARN			BT_CMD_START "R WRN"
#define BT_CMD_ERROR			BT_CMD_START "R ERR"

/* Dongle Replies */
#define DONGLE_CMD_OK			DONGLE_CMD_START "R OK"
#define DONGLE_CMD_MSG			DONGLE_CMD_START "R MSG"
#define DONGLE_CMD_WARN			DONGLE_CMD_START "R WRN"
#define DONGLE_CMD_ERROR		DONGLE_CMD_START "R ERR"

/* FLARM Replies */
#ifdef FLARM
#define FLARM_CMD_OK			FLARM_CMD_START "R OK"
#define FLARM_CMD_ERROR			FLARM_CMD_START "R ERR"
#endif

/* Commands */
/* FANET */
#define CMD_STATE			'S'
#define CMD_TRANSMIT			'T'
#define CMD_ADDR			'A'
#define CMD_CONFIG			'C'

#define CMD_RX_FRAME			"F"

#define CMD_NAME			'N'		//bluetooth only

/* Dongle */
#define CMD_VERSION			'V'
#define CMD_POWER			'P'
#define CMD_REGION			'L'
#define CMD_BOOTLOADER			'J'

/* FLARM */
#ifdef FLARM
#define CMD_EXPIRES			'X'
#endif


#define SERIAL_debug_mode		0

/* values <= 0 removes code and message */
#define FN_REPLY_OK			FANET_CMD_OK, 	 0,  ""
#define FN_REPLYM_INITIALIZED		FANET_CMD_MSG,   1,  "initialized"
#define FN_REPLYE_RADIO_FAILED		FANET_CMD_ERROR, 2,  "radio failed"
#define FN_REPLYE_UNKNOWN_CMD		FANET_CMD_ERROR, 5,  "unknown command"
#define FN_REPLYE_FN_UNKNOWN_CMD	FANET_CMD_ERROR, 6,  "unknown FN command"
#define FN_REPLYE_NO_SRC_ADDR		FANET_CMD_ERROR, 10, "no source address"
#define FN_REPLYE_INVALID_ADDR		FANET_CMD_ERROR, 11, "invalid address"
#define FN_REPLYE_INCOMPATIBLE_TYPE	FANET_CMD_ERROR, 12, "incompatible type"
#define FN_REPLYM_DOWNDOWN		FANET_CMD_MSG,	 13, "power down"
#define FN_REPLYE_TX_BUFF_FULL		FANET_CMD_ERROR, 14, "tx buffer full"
#define FN_REPLYE_ADDR_GIVEN		FANET_CMD_ERROR, 15, "address already set"
#define FN_REPLYE_CMD_TOO_SHORT		FANET_CMD_ERROR, 30, "too short"
#define FN_REPLYE_BT_FAILED		BT_CMD_ERROR,    51, "bt failed"
#define FN_REPLYE_BT_UNKNOWN_CMD	BT_CMD_ERROR,    52, "unknown BT command"
#define FN_REPLYE_BT_NAME_TOO_SHORT	BT_CMD_ERROR,    53, "name too short"
#define FN_REPLYW_BT_RECONF_ID		BT_CMD_WARN,     55, "reconfiguring id"
#define DN_REPLY_OK			DONGLE_CMD_OK, 	 0,  ""
#define DN_REPLYE_DONGLE_UNKNOWN_CMD	DONGLE_CMD_ERROR,60, "unknown DG command"
#define DN_REPLYE_JUMP			DONGLE_CMD_ERROR,61, "unknown jump point"
#define DN_REPLYE_POWER			DONGLE_CMD_ERROR,70, "power switch failed"
#define DN_REPLYE_TOOLESSPARAMETER	DONGLE_CMD_ERROR,80, "too less parameter"
#define DN_REPLYE_UNKNOWNPARAMETER	DONGLE_CMD_ERROR,81, "unknown parameter"
#ifdef FLARM
#define FA_REPLY_OK			FLARM_CMD_OK, 	 0,  ""
#define FA_REPLYE_UNKNOWN_CMD		FLARM_CMD_ERROR, 90, "unknown FLARM command"
#define FA_REPLYE_EXPIRED		FLARM_CMD_ERROR, 91, "FLARM expired"
#endif
/*
 * Normal Commands
 * State: 		#FNS lat(deg),lon(deg),alt(m),speed(km/h),climb(m/s),heading(deg)[,year(since 1900),month(0-11),day,hour,min,sec,turn(deg/s)]
 * 					note: all values in float/int (NOT hex), time is required for FLARM in struct tm format
 * Transmit: 		#FNT type,dest_manufacturer,dest_id,forward,ack_required,length,length*2hex		note: all values in hex
 *
 * Address: 		#FNA manufacturer(hex),id(hex)								note: w/o address is returned
 * Config: 		#FNC type(0..7),onlinelogging(0..1)							note: type see protocol.txt
 *
 *
 * Receive a Frame:	#FNF src_manufacturer,src_id,broadcast,signature,type,payloadlength,payload
 *
 * Maintenance/Dongle
 * Version:		#DGV
 * Power:		#DGP powermode(0..1)									note: w/o status is returned
 * Region:		#DGL freq(868,915),power(2..20 (dBm))							note: 10dBm is sufficient for
 * 															Skytraxx modules using the
 * 															stock antenna
 *
 * Jump to DFU:		#DGJ BLstm										stm32 default bootloader
 * 			#DGJ BLxld										xloader
 * 														recommended: use 2 GPIOs connected
 * 															to RESEST and BOOT0
 *
 * FLARM
 * Power		#FAP powermode(0..1)									note: w/o status is returned
 * Expiration Data:	#FAX					-> #FAX year,month,day	in struct tm format (decimal)
 *
 *
 * Bluetooth Commands
 * Name:		#BTN name
 * Address:		#BTA addr										note: cmd may be disabled
 */

class Serial_Interface
{
private:
	serial_t *myserial = NULL;
	uint32_t last_activity = 0;

	/* Normal Commands */
	void fanet_eval(char *str);
	void fanet_cmd_state(char *ch_str);
	void fanet_cmd_addr(char *ch_str);
	void fanet_cmd_config(char *ch_str);
	void fanet_cmd_transmit(char *ch_str);

	/* Dongle Commands */
	void dongle_eval(char *str);
	void dongle_cmd_version(char *ch_str);
	void dongle_cmd_power(char *ch_str);
	void dongle_cmd_region(char *ch_str);
	void dongle_cmd_jump(char *ch_str);

	/* Bluetooth Commands */
#ifdef FANET_BLUETOOTH
	void bt_eval(char *str);
	void bt_cmd_addr(char *ch_str);
	void bt_cmd_name(char *ch_str);
#endif

	/* FLARM Commands */
#ifdef FLARM
	void flarm_eval(char *str);
	void flarm_cmd_expires(char *ch_str);
	void flarm_cmd_power(char *ch_str);
#endif

public:
	Serial_Interface() {};
	void begin(serial_t *serial);

	/* redirected from app */
	void handle_frame(Frame *frm);
	void handle_acked(bool ack, MacAddr &addr);

	void handle_rx(void);
	void ack(bool value, int manufacturer, int id);

	uint32_t get_lastactivity(void) { return last_activity; };
	bool any_actitity(void) { return (last_activity!=0); };

	void flarm_expired(void){ print_line(FA_REPLYE_EXPIRED); };

	void print_line(const char *type, int key, const char *msg);
	void print(char *str);
};

extern Serial_Interface serial_int;

#endif /* SERIAL_H_ */
