/*
 * serial_fanet.c
 *
 *  Created on: May 21, 2017
 *      Author: sid
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "serial.h"
#include "cmd_fanet.h"

/* State: #FNS lat(deg),lon(deg),alt(m),speed(km/h),climb(m/s),heading(deg)[,turn(deg/s)] */
//note: all values in float (NOT hex)
void fanet_cmd_state(char *ch_str)
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
		serial_reply(FN_REPLYE_CMD_TOO_SHORT);
		return;
	}
	float lon = atof(++p);
	p = strchr(p, SEPARATOR);
	if(p == NULL)
	{
		serial_reply(FN_REPLYE_CMD_TOO_SHORT);
		return;
	}
	float alt = atof(++p);
	p = strchr(p, SEPARATOR);
	if(p == NULL)
	{
		serial_reply(FN_REPLYE_CMD_TOO_SHORT);
		return;
	}
	float speed = atof(++p);
	p = strchr(p, SEPARATOR);
	if(p == NULL)
	{
		serial_reply(FN_REPLYE_CMD_TOO_SHORT);
		return;
	}
	float climb = atof(++p);
	p = strchr(p, SEPARATOR);
	if(p == NULL)
	{
		serial_reply(FN_REPLYE_CMD_TOO_SHORT);
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

	printf("%f,%f,%f,%f,%f,%f,%f\n", lat, lon, alt, speed, climb, heading, turn);
	//todo!!!
	//app.set(lat, lon, alt, speed, climb, heading, turn);

	/* The state is only of interest if a src addr is set */
//	if(fmac.my_addr == MacAddr())
//		serial_reply(FN_REPLYE_NO_SRC_ADDR);
//	else
		serial_reply(FN_REPLY_OK);
}

/* Address: #FNA manufacturer(hex),id(hex) */
void fanet_cmd_addr(char *ch_str)
{
	/* address */
	char *p = (char *)ch_str;
	int manufacturer = strtol(p, NULL, 16);
	p = strchr(p, SEPARATOR)+1;
	int id = strtol(p, NULL, 16);

	if(manufacturer<=0 || manufacturer >= 0xFE || id <= 0 || id >= 0xFFFF)
	{
		serial_reply(FN_REPLYE_INVALID_ADDR);
	}
	else
	{
//TODO
//		fmac.my_addr.manufacturer = manufacturer;
//		fmac.my_addr.id = id;
		serial_reply(FN_REPLY_OK);
	}
}

/* Config: #FNC type(0..7),onlinelogging(0..1) */
void fanet_cmd_config(char *ch_str)
{
	/* config */
	char *p = (char *)ch_str;
	int type = strtol(p, NULL, 16);
	p = strchr(p, SEPARATOR)+1;
	int logging = strtol(p, NULL, 16);

	if(type < 0 || type > 7)
	{
		serial_reply(FN_REPLYE_INCOMPATIBLE_TYPE);
	}
	else
	{
//TODO
//		app.aircraft_type = type;
//		app.do_online_tracking = !!logging;
if(!!logging)//only to avoid compile errors
		serial_reply(FN_REPLY_OK);
	}
}

/* Transmit: #FNT type,dest_manufacturer,dest_id,forward,ack_required,length,length*2hex */
//note: all in HEX
void fanet_cmd_transmit(char *ch_str)
{
	/* w/o an address we can not tx */
//todo
//	if(fmac.my_addr == MacAddr())
//	{
//		serial_reply(FN_REPLYE_NO_SRC_ADDR);
//		return;
//	}

//	Frame *frm = new Frame(fmac.my_addr);

	/* header */
//	char *p = (char *)ch_str;
//	frm->type = strtol(p, NULL, 16);
//	p = strchr(p, SEPARATOR)+1;
//	frm->dest.manufacturer = strtol(p, NULL, 16);
//	p = strchr(p, SEPARATOR)+1;
//	frm->dest.id = strtol(p, NULL, 16);
//	p = strchr(p, SEPARATOR)+1;
//	frm->forward = !!strtol(p, NULL, 16);
//	p = strchr(p, SEPARATOR)+1;
	/* ACK required */
//	if(strtol(p, NULL, 16))
//	{
//		frm->ack_requested = frm->forward?MAC_ACK_TWOHOP:MAC_ACK_SINGLEHOP;
//		frm->num_tx = MAC_TX_RETRANSMISSION_RETRYS;
//	}
//	else
//	{
//		frm->ack_requested = MAC_NOACK;
//		frm->num_tx = 0;
//	}

	/* payload */
//	p = strchr(p, SEPARATOR)+1;
//	frm->payload_length = strtol(p, NULL, 16);
//	frm->payload = new uint8_t[frm->payload_length];

//	p = strchr(p, SEPARATOR)+1;
//	for(int i=0; i<frm->payload_length; i++)
//	{
//		char sstr[3] = {p[i*2], p[i*2+1], '\0'};
//		frm->payload[i] = strtol(sstr,  NULL,  16);
//	}

	/* pass to mac */
//	if(fmac.transmit(frm) == 0)
//	{
//		println(FANET_CMD_OK, 0, NULL);
//#ifdef FANET_NAME_AUTOBRDCAST
//		if(frm->type == FRM_TYPE_NAME)
//			app.allow_brdcast_name(false);
//#endif
//	}
//	else
//	{
//		delete frm;
		serial_reply(FN_REPLYE_TX_BUFF_FULL);
//	}
}


/* Eval fanet command */
void cmd_fanet_mux(char *cmd)
{
	int cmd_len = strlen(FANET_CMD_START);
	switch(cmd[cmd_len])
	{
	case CMD_FANET_STATE:
		fanet_cmd_state(&cmd[cmd_len]+1);
		break;
	case CMD_FANET_TRANSMIT:
		fanet_cmd_transmit(&cmd[cmd_len]+1);
		break;
	case CMD_FANET_ADDR:
		fanet_cmd_addr(&cmd[cmd_len]+1);
		break;
	case CMD_FANET_CONFIG:
		fanet_cmd_config(&cmd[cmd_len]+1);
		break;
	default:
		serial_reply(FN_REPLYE_FN_UNKNOWN_CMD);
	}
}
