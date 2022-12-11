/*
 * serial.cpp
 *
 *  Created on: 1 Oct 2016
 *      Author: sid
 */

#include <stdio.h>
#include <string>
#include <time.h>

#include "jump.h"
#include "serial.h"

#include "../fanet.h"
#include "../app.h"
#include "../radio/fmac.h"
#include "../radio/sx1272.h"
#ifdef FANET_BLUETOOTH
	#include "bm78.h"
#endif
#ifdef FLARM
	#include "../../flarm/caswrapper.h"
#endif
#include "serial_interface.h"



/* Fanet Commands */

/*
 * State: 		#FNS lat(deg),lon(deg),alt(m MSL),speed(km/h),climb(m/s),heading(deg)
 * 						[,year(since 1900),month(0-11),day,hour,min,sec,sep(m)[,turnrate(deg/sec)[,QNEoffset(m)]]]
 * 					note: all values in float/int (NOT hex), time is required for FLARM in struct tm format
 * 					note2: FLARM uses the ellipsoid altitudes ->
 * 							sep = Height of geoid (mean sea level) above WGS84 ellipsoid
 * 					note3: QNEoffset is optional: QNEoffset = QNE - GPS altitude
 */
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

	/* time stamp */
	struct tm t = {0};
	p = strchr(p, SEPARATOR);	//year
	if(p)
		t.tm_year = atoi(++p);
	p = strchr(p, SEPARATOR);	//month
	if(p)
		t.tm_mon = atoi(++p);
	p = strchr(p, SEPARATOR);	//day
	if(p)
		t.tm_mday = atoi(++p);
	p = strchr(p, SEPARATOR);	//hour
	if(p)
		t.tm_hour = atoi(++p);
	p = strchr(p, SEPARATOR);	//min
	if(p)
		t.tm_min = atoi(++p);
	p = strchr(p, SEPARATOR);	//sec
	if(p)
		t.tm_sec = atoi(++p);

	/* Geoid separation */
	float sep = NAN;
	p = strchr(p, SEPARATOR);
	if(p)
		sep = atof(++p);

	/* turn rate */
	float turnrate = NAN;
	if(p)				//we got sep -> continue
		p = strchr(p, SEPARATOR);
	if(p)
		turnrate = atof(++p);

	/* QNE offset */
	float qneOffset = NAN;
	if(p)				//we got sep -> continue
		p = strchr(p, SEPARATOR);
	if(p)
		qneOffset = atof(++p);

	/* ensure heading in [0..360] */
	while(heading > 360.0f)
		heading -= 360.0f;
	while(heading < 0.0f)
		heading += 360.0f;

	app.set(lat, lon, alt, speed, climb, heading, turnrate, qneOffset);

#ifdef FLARM
	CasWrapper::gpsdata_t gnss = {0};
	gnss.lat = lat;
	gnss.lon = lon;
	gnss.alt = alt;
	gnss.sep = sep;
	gnss.speed_kmh = speed;
	gnss.climb = climb;
	gnss.heading = heading;
	gnss.t = t;
	gnss.valid = true;

	CasWrapper::getInstance().updateGnss(HAL_GetTick(), gnss);
#else
	if(t.tm_sec)
	{
		//use t to prevent compiler error...
	}
#endif


	/* The state is only of interest if a src addr is set */
	if(fmac.myAddr == MacAddr())
		print_line(FN_REPLYE_NO_SRC_ADDR);
	else if(!sx1272_isArmed())
		print_line(FN_REPLYM_PWRDOWN);
	else
		print_line(FN_REPLY_OK);
}

/* Address: #FNA manufacturer(hex),id(hex) */
void Serial_Interface::fanet_cmd_addr(char *ch_str)
{
#if defined(SerialDEBUG) && (SERIAL_debug_mode > 0)
	SerialDEBUG.print(F("### Addr "));
	SerialDEBUG.print(ch_str);
#endif
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
		/* report addr */
		char buf[64];
		snprintf(buf, sizeof(buf), "%s%c %02X,%04X\n", FANET_CMD_START, CMD_ADDR, fmac.myAddr.manufacturer, fmac.myAddr.id);
		print(buf);
		return;
	}

	if(strstr(ch_str, "ERASE!") != NULL)
	{
		/* erase config */
		//must never be used by the end user
		if(fmac.eraseAddr())
			print_line(FN_REPLY_OK);
		else
			print_line(FN_REPLYE_FN_UNKNOWN_CMD);
		return;
	}

	/* address */
	char *p = (char *)ch_str;
	int manufacturer = strtol(p, NULL, 16);
	p = strchr(p, SEPARATOR)+1;
	int id = strtol(p, NULL, 16);

	if(manufacturer<=0 || manufacturer >= 0xFE || id <= 0 || id >= 0xFFFF)
	{
		print_line(FN_REPLYE_INVALID_ADDR);
	}
	else if(fmac.setAddr(MacAddr(manufacturer, id)))
	{
#ifdef FLARM
		//note: for now we ignore the manufacturer
		CasWrapper::getInstance().setAddress(id);
#endif
		print_line(FN_REPLY_OK);
	}
	else
	{
		print_line(FN_REPLYE_ADDR_GIVEN);
	}
}

/* Config: #FNC airType(0..7),onlinelogging(0..1)[,groundType(0..F in hex)] */
void Serial_Interface::fanet_cmd_config(char *ch_str)
{
#if SERIAL_debug_mode > 0
	printf("### Config %s", ch_str);
#endif
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
		/* report addr */
		char buf[64];
		snprintf(buf, sizeof(buf), "%s%c %X,%X,%X\n", FANET_CMD_START, CMD_CONFIG, app.aircraft, app.doOnlineTracking, app.state);
		print(buf);
		return;
	}

	/* config */
	char *p = (char *)ch_str;
	App::aircraft_t type = static_cast<App::aircraft_t>(strtol(p, NULL, 16));
	p = strchr(p, SEPARATOR)+1;
	int logging = strtol(p, NULL, 16);

	/* optional ground state */
	if((p = strchr(p, SEPARATOR)) != nullptr)
		app.state = static_cast<App::status_t>(strtol(++p, NULL, 16));

	if(type < 0 || type > 7)
	{
		print_line(FN_REPLYE_INCOMPATIBLE_TYPE);
	}
	else
	{
#ifdef FLARM
		/* set FLARM type */
		if(type == App::paraglider)
			CasWrapper::getInstance().setAircraftType(AT_SOFT_HG);
		else if(type == App::hangglider)
			CasWrapper::getInstance().setAircraftType(AT_FIXED_HG);
#endif

		/* set FANET type */
		app.aircraft = type;
		app.doOnlineTracking = !!logging;
		print_line(FN_REPLY_OK);
	}
}

/* Config: #FNM 0..1 */
void Serial_Interface::fanet_cmd_mode(char *ch_str)
{
#if SERIAL_debug_mode > 0
	printf("### Mode %s", ch_str);
#endif
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
		/* report addr */
		char buf[64];
		snprintf(buf, sizeof(buf), "%s%c %X\n", FANET_CMD_START, CMD_MODE, app.onGround);
		print(buf);
		return;
	}

	if(*ch_str == '0' || *ch_str == '1')
	{
		app.onGround = !!atoi(ch_str);
		print_line(FN_REPLY_OK);
	}
	else
	{
		print_line(FN_REPLYE_INCOMPATIBLE_TYPE);
	}
}

/* Transmit: #FNT type,dest_manufacturer,dest_id,forward,ack_required,length,length*2hex[,signature] */
//note: all in HEX
extern uint8_t sx_reg_backup[2][111];
void Serial_Interface::fanet_cmd_transmit(char *ch_str)
{
#if SERIAL_debug_mode > 0
	printf("### Packet %s\n", ch_str);
#endif

	/* remove \r\n and any spaces */
	char *ptr = strchr(ch_str, '\r');
	if(ptr == nullptr)
		ptr = strchr(ch_str, '\n');
	if(ptr != nullptr)
		*ptr = '\0';
	while(*ch_str == ' ')
		ch_str++;

	/* integrity check */
	for(char *ptr = ch_str; *ptr != '\0'; ptr++)
	{
		if(*ptr >= '0' && *ptr <= '9')
			continue;
		if(*ptr >= 'A' && *ptr <= 'F')
			continue;
		if(*ptr >= 'a' && *ptr <= 'f')
			continue;
		if(*ptr == ',')
			continue;

		/* not allowed char */
		print_line(FN_REPLYE_CMD_BROKEN);
		return;
	}

	/* w/o an address we can not tx */
	if(fmac.myAddr == MacAddr())
	{
		print_line(FN_REPLYE_NO_SRC_ADDR);
		return;
	}

	/* no need to generate a package. tx queue is full */
	if(!fmac.txQueueHasFreeSlots())
	{
		print_line(FN_REPLYE_TX_BUFF_FULL);
		return;
	}

	Frame *frm = new Frame(fmac.myAddr);

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
		frm->ack_requested = frm->forward?FRM_ACK_TWOHOP:FRM_ACK_SINGLEHOP;
		frm->num_tx = MAC_TX_RETRANSMISSION_RETRYS;
	}
	else
	{
		frm->ack_requested = FRM_NOACK;
		frm->num_tx = 0;
	}

	/* payload */
	p = strchr(p, SEPARATOR)+1;
	frm->payload_length = strtol(p, NULL, 16);
	if(frm->payload_length >= 128)
	{
		delete frm;
		print_line(FN_REPLYE_FRM_TOO_LONG);
		return;
	}
	frm->payload = new uint8_t[frm->payload_length];

	p = strchr(p, SEPARATOR)+1;
	for(int i=0; i<frm->payload_length; i++)
	{
		char sstr[3] = {p[i*2], p[i*2+1], '\0'};
		if(strlen(sstr) != 2)
		{
			print_line(FN_REPLYE_CMD_TOO_SHORT);
			delete frm;
			return;
		}
		frm->payload[i] = strtol(sstr,  NULL,  16);
	}

	/* signature */
	if((p = strchr(p, SEPARATOR)) != NULL)
		frm->signature = ((uint32_t)strtoll(++p, NULL, 16));

	/* pass to mac */
	if(fmac.transmit(frm) == 0)
	{
		if(!sx1272_isArmed())
			print_line(FN_REPLYM_PWRDOWN);
		else
			print_line(FN_REPLY_OK);
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

/* Address: #FNZ [zoneId[0..6]], note return meight contain -1 -> unset */
void Serial_Interface::fanet_cmd_zone(char *ch_str)
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
		/* report addr */
		char buf[64];
		snprintf(buf, sizeof(buf), "%s%c %d,%s\n", FANET_CMD_START, CMD_ZONE,
				fmac.getZone(), fmac.getZone()>=0 ? fmac.zones[fmac.getZone()].name : "UNSET");
		print(buf);
		return;
	}

	/* idx */
	char *p = (char *)ch_str;
	int idx = strtol(p, NULL, 10);

	if(idx<=0 or idx >= (int)NELEM(fmac.zones))
	{
		print_line(FN_REPLYE_INCOMPATIBLE_TYPE);
		return;
	}

	fmac.setZone(idx);
	print_line(FN_REPLY_OK);
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
	case CMD_MODE:
		fanet_cmd_mode(&str[strlen(DONGLE_CMD_START) + 1]);
		break;
	case CMD_ZONE:
		fanet_cmd_zone(&str[strlen(DONGLE_CMD_START) + 1]);
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

#ifdef FLARM
	if(!atoi(ch_str))
		CasWrapper::getInstance().do_flarm = false;
#endif

	/* set status */
	if(fmac.setPower(!!atoi(ch_str)))
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

void Serial_Interface::dongle_pps(void)
{
	/* emulate pps signal */
	fanet_pps_int();
	print_line(DN_REPLY_OK);
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
	case CMD_PPS:
		dongle_pps();
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
	if(fmac.myAddr.id != 0)
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

/*
 * FLARM
 */
#ifdef FLARM

void Serial_Interface::flarm_cmd_power(char *ch_str)
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
		snprintf(buf, sizeof(buf), "%s%c %d\n", FLARM_CMD_START, CMD_POWER, CasWrapper::getInstance().do_flarm);
		print(buf);
		return;
	}


	/* enable rf backend if not yet powered up already */
	if(!!atoi(ch_str) && !sx1272_isArmed())
		fmac.setPower(true);

	/* set status */
	CasWrapper::getInstance().do_flarm = !!atoi(ch_str);
	print_line(FA_REPLY_OK);
}

void Serial_Interface::flarm_cmd_expires(char *ch_str)
{
	if(myserial == NULL)
		return;

	const time_t expt = CasWrapper::getInstance().getExpirationDate();
	struct tm t;
	gmtime_r(&expt, &t);

	char buf[64];
	snprintf(buf, sizeof(buf), "%s%c %d,%d,%d\n", FLARM_CMD_START, CMD_EXPIRES, t.tm_year, t.tm_mon, t.tm_mday);
	print(buf);
}

/* mux string */
void Serial_Interface::flarm_eval(char *str)
{
	switch(str[strlen(FLARM_CMD_START)])
	{
	case CMD_EXPIRES:
		flarm_cmd_expires(&str[strlen(FLARM_CMD_START) + 1]);
		break;
	case CMD_POWER:
		flarm_cmd_power(&str[strlen(FLARM_CMD_START) + 1]);
		break;
	default:
		print_line(FA_REPLYE_UNKNOWN_CMD);
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
		print((char *)"### rx:'");
		print(line);
		print((char *)"'\n");
#endif
		if(!strncmp(line, FANET_CMD_START, 3))
			fanet_eval(line);
#ifdef FANET_BLUETOOTH
		else if(!strncmp(line, BT_CMD_START,3))
			bt_eval(line);
#endif
		else if(!strncmp(line, DONGLE_CMD_START, 3))
			dongle_eval(line);
#ifdef FLARM
		else if(!strncmp(line, FLARM_CMD_START, 3))
			flarm_eval(line);
#endif
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
	snprintf(buf, sizeof(buf), "%s %X,%X,%X,%X,%X,%X,",
			FANET_CMD_START CMD_RX_FRAME, frm->src.manufacturer, frm->src.id, frm->dest==MacAddr(), (unsigned int)frm->signature,
			frm->type, frm->payload_length);
	print(buf);

	for(int i=0; i<frm->payload_length; i++)
	{
		snprintf(buf, sizeof(buf), "%02X", frm->payload[i]);
		print(buf);
	}
	print((char*) "\n");
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

