/*
 * app.h
 *
 *  Created on: 17 Oct 2016
 *      Author: sid
 */

#ifndef STACK_APP_H_
#define STACK_APP_H_

#include "radio/fmac.h"
#include "wire/serial_interface.h"

#define APP_VALID_STATE_MS			10000

#define APP_TYPE1OR7_AIRTIME_MS			40		//actually 20-30ms
#define	APP_TYPE1OR7_MINTAU_MS			250
#define	APP_TYPE1OR7_TAU_MS			5000

#define APP_TYPE1_SIZE				12
#define APP_TYPE7_SIZE				7

class App : public Fapp
{
public:
	enum aircraft_t : uint8_t
	{
		otherAircraft = 0,
		paraglider = 1,
		hangglider = 2,
		balloon = 3,
		glider = 4,
		poweredAircraft = 5,
		helicopter = 6,
		uav = 7,
	};

	enum status_t : uint16_t
	{
		otherStatus = 0,
		hiking = 1,
		vehilce = 2,
		bike = 3,
		boot = 4,
		needAride = 8,

		needTechnicalAssistance = 12,
		needMedicalHelp = 13,
		distressCall = 14,
		distressCallAuto = 15,				//max number
	};

private:
	/* units are degrees, seconds, and meter */
	float latitude = NAN;
	float longitude = NAN;
	int altitude;
	float speed;
	float climb;
	float heading;
	float turnrate;

	Serial_Interface *mySerialInt = NULL;

	/* ensures the broadcasted information are still valid */
	unsigned long valid_until = 0;

	/* determines the tx rate */
	unsigned long last_tx = 0;
	unsigned long next_tx = 0;

#ifdef FANET_NAME_AUTOBRDCAST
	char name[20] = "\0";
	bool brdcast_name = false;

	int serialize_name(uint8_t*& buffer)
	{
		const int namelength = strlen(name);
		buffer = new uint8_t[namelength];
		memcpy(buffer, name, namelength);
		return namelength;
	}
#endif

	int serializeTracking(uint8_t*& buffer);
	int serializeGroundTracking(uint8_t*& buffer);

public:
	bool doOnlineTracking = true;
	bool onGround = false;
	aircraft_t aircraft = paraglider;
	status_t state = hiking;

	void set(float lat, float lon, float alt, float speed, float climb, float heading, float turn);

	/* device -> air */
	bool is_broadcast_ready(int num_neighbors);
	void broadcast_successful(int type) { last_tx = HAL_GetTick(); }

	/* air -> device */
	void handle_acked(bool ack, MacAddr &addr) { if(mySerialInt == NULL) return; mySerialInt->handle_acked(ack, addr); }
	void handle_frame(Frame *frm) {	if(mySerialInt == NULL)	return; mySerialInt->handle_frame(frm);	}

	void begin(Serial_Interface &si) { mySerialInt = &si; }
	Frame *get_frame();

#ifdef FANET_NAME_AUTOBRDCAST
	/* Name automation in case the host application does not know this... */
	void set_name(char *devname) { snprintf(name, sizeof(name), devname); };
	void allow_brdcast_name(boolean value)
	{
		if(value == false)
			brdcast_name = false;
		else
			brdcast_name = (strlen(name)==0?false:true);
	};
#endif
};


extern App app;

#endif /* STACK_APP_H_ */
