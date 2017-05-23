/*
 * serial.cpp
 *
 *  Created on: 1 Oct 2016
 *      Author: sid
 */

#include <stdio.h>
#include <string>


#include "jump.h"
#include "serial.h"

#include "../fanet.h"
#include "../app.h"
#include "../radio/fmac.h"
#include "../radio/sx1272.h"
#ifdef FANET_BLUETOOTH
	#include "bm78.h"
#endif
#include "serial_interface.h"


/* Fanet Commands */

/* State: #FNS lat(deg),lon(deg),alt(m),speed(km/h),climb(m/s),heading(deg)[,turn(deg/s)] */
//note: all values in float (NOT hex)
void Serial_Interface::fanet_cmd_state(char *ch_str)
{
#if defined(SerialDEBUG) && (SERIAL_debug_mode > 0)
	SerialDEBUG.print(F("### State "));
	SerialDEBUG.print(ch_str);
#endif
	/* state */
	char *p = (char *)ch_str;
	float lat = atof(p);
	p = strchr(p, SEPARATOR);
	if(p == NULL)
	{
		print_line(FN_REPLYE_CMD_TOO_SHORT);
		return;
	}
	float lon = atof(++p);
	p = strchr(p, SEPARATOR);
	if(p == NULL)
	{
		print_line(FN_REPLYE_CMD_TOO_SHORT);
		return;
	}
	float alt = atof(++p);
	p = strchr(p, SEPARATOR);
	if(p == NULL)
	{
		print_line(FN_REPLYE_CMD_TOO_SHORT);
		return;
	}
	float speed = atof(++p);
	p = strchr(p, SEPARATOR);
	if(p == NULL)
	{
		print_line(FN_REPLYE_CMD_TOO_SHORT);
		return;
	}
	float climb = atof(++p);
	p = strchr(p, SEPARATOR);
	if(p == NULL)
	{
		print_line(FN_REPLYE_CMD_TOO_SHORT);
		return;
	}
	float heading = atof(++p);
	p = strchr(p, SEPARATOR);
	float turn = NAN;
	if(p)
		turn = atof(++p);

	/* ensure heading in [0..360] */
	while(heading > 360.0f)
		heading -= 360.0f;
	while(heading < 0.0f)
		heading += 360.0f;

	app.set(lat, lon, alt, speed, climb, heading, turn);

	/* The state is only of interest if a src addr is set */
	if(fmac.my_addr == MacAddr())
	{
		print_line(FN_REPLYE_NO_SRC_ADDR);
	}
	else
	{
		print_line(FN_REPLY_OK);
	}
}

/* Address: #FNA manufacturer(hex),id(hex) */
void Serial_Interface::fanet_cmd_addr(char *ch_str)
{
#if defined(SerialDEBUG) && (SERIAL_debug_mode > 0)
	SerialDEBUG.print(F("### Addr "));
	SerialDEBUG.print(ch_str);
#endif
	/* address */
	char *p = (char *)ch_str;
	int manufacturer = strtol(p, NULL, 16);
	p = strchr(p, SEPARATOR)+1;
	int id = strtol(p, NULL, 16);

	if(manufacturer<=0 || manufacturer >= 0xFE || id <= 0 || id >= 0xFFFF)
	{
		print_line(FN_REPLYE_INVALID_ADDR);
	}
	else
	{
		fmac.my_addr.manufacturer = manufacturer;
		fmac.my_addr.id = id;
		print_line(FN_REPLY_OK);
	}
}

/* Config: #FNC type(0..7),onlinelogging(0..1) */
void Serial_Interface::fanet_cmd_config(char *ch_str)
{
#if defined(SerialDEBUG) && (SERIAL_debug_mode > 0)
	SerialDEBUG.print(F("### Config "));
	SerialDEBUG.print(ch_str);
#endif
	/* config */
	char *p = (char *)ch_str;
	int type = strtol(p, NULL, 16);
	p = strchr(p, SEPARATOR)+1;
	int logging = strtol(p, NULL, 16);

	if(type < 0 || type > 7)
	{
		print_line(FN_REPLYE_INCOMPATIBLE_TYPE);
	}
	else
	{
		app.aircraft_type = type;
		app.do_online_tracking = !!logging;
		print_line(FN_REPLY_OK);
	}
}

/* Transmit: #FNT type,dest_manufacturer,dest_id,forward,ack_required,length,length*2hex */
//note: all in HEX
void Serial_Interface::fanet_cmd_transmit(char *ch_str)
{
#if defined(SerialDEBUG) && (SERIAL_debug_mode > 0)
	SerialDEBUG.print(F("### Packet "));
	SerialDEBUG.print(ch_str);
#endif

	/* w/o an address we can not tx */
	if(fmac.my_addr == MacAddr())
	{
		print_line(FN_REPLYE_NO_SRC_ADDR);
		return;
	}

	Frame *frm = new Frame(fmac.my_addr);

	/* header */
	char *p = (char *)ch_str;
	frm->type = strtol(p, NULL, 16);
	p = strchr(p, SEPARATOR)+1;
	frm->dest.manufacturer = strtol(p, NULL, 16);
	p = strchr(p, SEPARATOR)+1;
	frm->dest.id = strtol(p, NULL, 16);
	p = strchr(p, SEPARATOR)+1;
	frm->forward = !!strtol(p, NULL, 16);
	p = strchr(p, SEPARATOR)+1;
	/* ACK required */
	if(strtol(p, NULL, 16))
	{
		frm->ack_requested = frm->forward?MAC_ACK_TWOHOP:MAC_ACK_SINGLEHOP;
		frm->num_tx = MAC_TX_RETRANSMISSION_RETRYS;
	}
	else
	{
		frm->ack_requested = MAC_NOACK;
		frm->num_tx = 0;
	}

	/* payload */
	p = strchr(p, SEPARATOR)+1;
	frm->payload_length = strtol(p, NULL, 16);
	frm->payload = new uint8_t[frm->payload_length];

	p = strchr(p, SEPARATOR)+1;
	for(int i=0; i<frm->payload_length; i++)
	{
		char sstr[3] = {p[i*2], p[i*2+1], '\0'};
		frm->payload[i] = strtol(sstr,  NULL,  16);
	}

	/* pass to mac */
	if(fmac.transmit(frm) == 0)
	{
		print_line(FANET_CMD_OK, 0, NULL);
#ifdef FANET_NAME_AUTOBRDCAST
		if(frm->type == FRM_TYPE_NAME)
			app.allow_brdcast_name(false);
#endif
	}
	else
	{
		delete frm;
		print_line(FN_REPLYE_TX_BUFF_FULL);
	}
}

/* mux string */
void Serial_Interface::fanet_eval(char *str)
{
	switch(str[strlen(FANET_CMD_START)])
	{
	case CMD_STATE:
		fanet_cmd_state(&str[strlen(DONGLE_CMD_START) + 1]);
		break;
	case CMD_TRANSMIT:
		fanet_cmd_transmit(&str[strlen(DONGLE_CMD_START) + 1]);
		break;
	case CMD_ADDR:
		fanet_cmd_addr(&str[strlen(DONGLE_CMD_START) + 1]);
		break;
	case CMD_CONFIG:
		fanet_cmd_config(&str[strlen(DONGLE_CMD_START) + 1]);
		break;
	default:
		print_line(FN_REPLYE_FN_UNKNOWN_CMD);
	}
}

/*
 * Dongle Stuff
 */

void Serial_Interface::dongle_cmd_version(char *ch_str)
{
	if(myserial == NULL)
		return;

	char buf[64];
	snprintf(buf, sizeof(buf), "%s%c %s\n", DONGLE_CMD_START, CMD_VERSION, BUILD);
	print(buf);
}

void Serial_Interface::dongle_cmd_jump(char *ch_str)
{
	if(!strncmp(ch_str, " BLxld", 6))
		jump_app((void *)FLASH_BASE);
	else if(!strncmp(ch_str, " BLstm", 6))
		jump_mcu_bootloader();

	print_line(DN_REPLYE_JUMP);
}

void Serial_Interface::dongle_cmd_power(char *ch_str)
{
	/* remove \r\n and any spaces*/
	char *ptr = strchr(ch_str, '\r');
	if(ptr == NULL)
		ptr = strchr(ch_str, '\n');
	if(ptr != NULL)
		*ptr = '\0';
	while(*ch_str == ' ')
		ch_str++;

	if(strlen(ch_str) == 0)
	{
		/* report armed state */
		char buf[64];
		snprintf(buf, sizeof(buf), "%s%c %X\n", DONGLE_CMD_START, CMD_POWER, sx1272_isArmed());
		print(buf);
		return;
	}

	/* set status */
	if(sx1272_setArmed(!!atoi(ch_str)))
		print_line(DN_REPLY_OK);
	else
		print_line(DN_REPLYE_POWER);
}

void Serial_Interface::dongle_cmd_region(char *ch_str)
{
#if defined(SerialDEBUG) && (SERIAL_debug_mode > 0)
	SerialDEBUG.print(F("### Region "));
	SerialDEBUG.print(ch_str);
#endif

	/* eval parameter */
	char *p = (char *)ch_str;
	if(p == NULL)
	{
		print_line(DN_REPLYE_TOOLESSPARAMETER);
		return;
	}
	int freq = strtol(p, NULL, 10);
	p = strchr(p, SEPARATOR)+1;
	if(p == NULL)
	{
		print_line(DN_REPLYE_TOOLESSPARAMETER);
		return;
	}
	sx_region_t region;
	region.channel = 0;
	region.dBm = strtol(p, NULL, 10);

	switch(freq)
	{
	case 868:
		region.channel = CH_868_200;
		break;
	/* ETSI test extension */
	case 225:
		region.channel = CH_868_225;
		break;
	case 250:
		region.channel = CH_868_250;
		break;
	case 275:
		region.channel = CH_868_275;
		break;
	case 300:
		region.channel = CH_868_300;
		break;
	case 325:
		region.channel = CH_868_325;
		break;
	case 350:
		region.channel = CH_868_350;
		break;
	case 375:
		region.channel = CH_868_375;
		break;
	case 400:
		region.channel = CH_868_400;
		break;

	}

	/* configure hardware */
	if(region.channel && sx1272_setRegion(region))
		print_line(DN_REPLY_OK);
	else
		print_line(DN_REPLYE_UNKNOWNPARAMETER);
}

/* mux string */
void Serial_Interface::dongle_eval(char *str)
{
	switch(str[strlen(DONGLE_CMD_START)])
	{
	case CMD_VERSION:
		dongle_cmd_version(&str[strlen(DONGLE_CMD_START) + 1]);
		break;
	case CMD_POWER:
		dongle_cmd_power(&str[strlen(DONGLE_CMD_START) + 1]);
		break;
	case CMD_REGION:
		dongle_cmd_region(&str[strlen(DONGLE_CMD_START) + 1]);
		break;
	case CMD_BOOTLOADER:
		dongle_cmd_jump(&str[strlen(DONGLE_CMD_START) + 1]);
		break;
	default:
		print_line(DN_REPLYE_DONGLE_UNKNOWN_CMD);
	}
}

/*
 * Bluetooth Commands
 */
#ifdef FANET_BLUETOOTH

/* Address: #BTA id(hex) */
void Serial_Interface::bt_cmd_addr(char *ch_str)
{
#if defined(SerialDEBUG) && (SERIAL_debug_mode > 0)
	SerialDEBUG.print(F("### BT id "));
	SerialDEBUG.print(ch_str);
#endif
	if(fmac.my_addr.id != 0)
		print_line(FN_REPLYW_BT_RECONF_ID);

	/* address */
	char *p = (char *)ch_str;
	int id = strtol(p, NULL, 16);

	if(id <= 0 || id >= 0xFFFF)
	{
		print_line(FN_REPLYE_INVALID_ADDR);
		return;
	}
	/* confirm before set value as we will loose the connection */
	mySerial->print_line(BT_CMD_OK);
	mySerial->flush();
	delay(500);

	bm78.write_id(id);
}

/* Address: #BTN name */
void Serial_Interface::bt_cmd_name(char *ch_str)
{
#if defined(SerialDEBUG) && (SERIAL_debug_mode > 0)
	SerialDEBUG.print(F("### BT Name "));
	SerialDEBUG.print(ch_str);
#endif

	/* remove trailing spaces */
	while(*ch_str == ' ')
		ch_str++;

	/* remove \r\n */
	char *ptr = strchr(ch_str, '\r');
 	if(ptr != NULL)
 		*ptr = '\0';
 	ptr = strchr(ch_str, '\n');
 	if(ptr != NULL)
 		*ptr = '\0';

	if(strlen(ch_str) == 0)
	{
		print_line(FN_REPLYE_BT_NAME_TOO_SHORT);
		return;
	}

	/* confirm before set value as we will loose the connection */
	mySerial->print_line(BT_CMD_OK);
	mySerial->flush();
	delay(500);

	bm78.write_name(ch_str);
}

void Serial_Interface::bt_eval(String &str)
{
	const char cmd = str.charAt(strlen(FANET_CMD_START));
	switch(cmd)
	{
#ifndef FANET_BLUETOOTH_ENDUSER_SAVE
	case CMD_ADDR:
		bt_cmd_addr((char *) str.substring(strlen(BT_CMD_START) + 1).c_str());
		break;
#endif
	case CMD_NAME:
		bt_cmd_name((char *) str.substring(strlen(BT_CMD_START) + 1).c_str());
		break;
	default:
		print_line(FN_REPYLE_BT_UNKNOWN_CMD);
	}
}

#endif

/* collect string */
void Serial_Interface::handle_rx(void)
{
	if(myserial == NULL)
		return;

	char line[256];
	bool cmd_rxd = serial_poll(myserial, line, sizeof(line));

	/* Process message */
	if (cmd_rxd)
	{
#if defined(SerialDEBUG) && (SERIAL_debug_mode > 1)
		SerialDEBUG.print(F("### rx:"));
		SerialDEBUG.print(rxStr);
#endif
		if(!strncmp(line, FANET_CMD_START, 3))
			fanet_eval(line);
#ifdef FANET_BLUETOOTH
		else if(!strncmp(line, BT_CMD_START,3))
			bt_eval(line);
#endif
		else if(!strncmp(line, DONGLE_CMD_START, 3))
			dongle_eval(line);
		else
			print_line(FN_REPLYE_UNKNOWN_CMD);

		last_activity = HAL_GetTick();
	}
}

void Serial_Interface::begin(serial_t *serial)
{
	myserial = serial;
}

/*
 * Handle redirected App Stuff
 */

void Serial_Interface::handle_acked(bool ack, MacAddr &addr)
{
	if(myserial == NULL)
		return;

	char buf[64];
	snprintf(buf, sizeof(buf), "%s,%X,%X\n", ack?FANET_CMD_ACK:FANET_CMD_NACK, addr.manufacturer, addr.id);
	print(buf);
}

void Serial_Interface::handle_frame(Frame *frm)
{
	if(myserial == NULL || frm == NULL)
		return;

	/* simply print frame */
	/* src_manufacturer,src_id,broadcast,signature,type,payloadlength,payload */

	char buf[128];
	snprintf(buf, sizeof(buf), "%s,%X,%X,%X,%X,%X,%X,",
			FANET_CMD_START CMD_RX_FRAME, frm->src.manufacturer, frm->src.id, frm->dest==MacAddr(), frm->signature,
			frm->type, frm->payload_length);
	print(buf);

	for(int i=0; i<frm->payload_length; i++)
	{
		snprintf(buf, sizeof(buf), "%02X", frm->payload[i]);
		print(buf);
	}
	prints("\n");
}

void Serial_Interface::print_line(const char *type, int key, const char *msg)
{
	if(myserial == NULL || type == NULL)
		return;

	/* general answer */
	while(HAL_UART_Transmit_IT(myserial->uart, (uint8_t *)type, strlen(type)) == HAL_BUSY);
	while(myserial->uart->gState != (__IO HAL_UART_StateTypeDef) HAL_UART_STATE_READY);

	/* key */
	char buf[64];
	if(key > 0)
	{
		size_t len = snprintf(buf, sizeof(buf), ",%d", key);
		while(HAL_UART_Transmit_IT(myserial->uart, (uint8_t *)buf, len) == HAL_BUSY);
		while(myserial->uart->gState != (__IO HAL_UART_StateTypeDef) HAL_UART_STATE_READY);

		/* human readable message */
		if(msg != NULL && strlen(msg) > 0)
		{
			len =  snprintf(buf, sizeof(buf), ",%s", msg);
			while(HAL_UART_Transmit_IT(myserial->uart, (uint8_t *)buf,len) == HAL_BUSY);
			while(myserial->uart->gState != (__IO HAL_UART_StateTypeDef) HAL_UART_STATE_READY);
		}
	}

	while(HAL_UART_Transmit_IT(myserial->uart, (uint8_t *)"\n", 1) == HAL_BUSY);
	while(myserial->uart->gState != (__IO HAL_UART_StateTypeDef) HAL_UART_STATE_READY);
}

void Serial_Interface::print(char *str)
{
	if(myserial == NULL || str == NULL)
		return;

	while(HAL_UART_Transmit_IT(myserial->uart, (uint8_t *)str, strlen(str)) == HAL_BUSY);
	while(myserial->uart->gState != (__IO HAL_UART_StateTypeDef) HAL_UART_STATE_READY);
}

Serial_Interface serial_int = Serial_Interface();

