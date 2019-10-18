/*
 * app.cpp
 *
 *  Created on: 8 Jan 2017
 *      Author: sid
 */

#include "constrain.h"

#include "app.h"

int App::serializeTracking(uint8_t*& buffer)
{
	buffer = new uint8_t[APP_TYPE1_SIZE];

	/* position */
	Frame::coord2payload_absolut(latitude, longitude, buffer);

	/* altitude set the lower 12bit */
	int alt = constrain(altitude, 0, 8190);
	if(alt > 2047)
		((uint16_t*)buffer)[3] = ((alt+2)/4) | (1<<11);				//set scale factor
	else
		((uint16_t*)buffer)[3] = alt;
	/* online tracking */
	((uint16_t*)buffer)[3] |= !!doOnlineTracking<<15;
	/* aircraft type */
	((uint16_t*)buffer)[3] |= (aircraft&0x7)<<12;

	/* Speed */
	int speed2 = constrain((int)roundf(speed*2.0f), 0, 635);
	if(speed2 > 127)
		buffer[8] = ((speed2+2)/5) | (1<<7);					//set scale factor
	else
		buffer[8] = speed2;

	/* Climb */
	int climb10 = constrain((int)roundf(climb*10.0f), -315, 315);
	if(abs(climb10) > 63)
		buffer[9] = ((climb10 + (climb10>=0?2:-2))/5) | (1<<7);			//set scale factor
	else
		buffer[9] = climb10 & 0x7F;

	/* Heading */
	buffer[10] = constrain((int)roundf(heading*256.0f/360.0f), 0, 255);

	/* Turn rate */
	if(!isnan(turnrate))
	{
		int turnr4 = constrain((int)roundf(turnrate*4.0f), -255, 255);
		if(abs(turnr4) > 63)
			buffer[11] = ((turnr4 + (turnr4>=0?2:-2))/4) | (1<<7);			//set scale factor
		else
			buffer[11] = turnr4 & 0x7f;
		return APP_TYPE1_SIZE;
	}
	else
	{
		return APP_TYPE1_SIZE - 1;
	}
}

int App::serializeGroundTracking(uint8_t*& buffer)
{
	buffer = new uint8_t[APP_TYPE7_SIZE];

	/* position */
	Frame::coord2payload_absolut(latitude, longitude, buffer);

	/* state */
	buffer[6] = (state&0x0F)<<4 | (!!doOnlineTracking);

	return APP_TYPE7_SIZE;
}

void App::set(float lat, float lon, float alt, float speed, float climb, float heading, float turn)
{
	/* currently only used in linear mode */
	//noInterrupts();

	latitude = lat;
	longitude = lon;
	altitude = roundf(alt);
	this->speed = speed;
	this->climb = climb;
	if(heading < 0.0f)
		heading += 360.0f;
	this->heading = heading;
	turnrate = turn;

	valid_until = HAL_GetTick() + APP_VALID_STATE_MS;

	//interrupts();
}

bool App::is_broadcast_ready(int num_neighbors)
{
	/* is the state valid? */
	if(HAL_GetTick() > valid_until || isnan(latitude) || isnan(longitude))
		return false;

	/* in case of a busy channel, ensure that frames from the fifo get also a change */
	if(next_tx > HAL_GetTick())
		return false;

	/* determine if its time to send something (again) */
	const int tau_add = (num_neighbors/10 + 1) * APP_TYPE1OR7_TAU_MS;
	if(last_tx + tau_add > HAL_GetTick())
		return false;

	return true;
}

Frame *App::get_frame()
{
	/* prepare frame */
	Frame *frm = new Frame(fmac.myAddr);
#ifdef FANET_NAME_AUTOBRDCAST
	static uint32_t framecount = 0;
	if(brdcast_name && (framecount & 0x7F) == 0)
	{
		/* broadcast name */
		frm->type = FRM_TYPE_NAME;
		frm->payload_length = serialize_name(frm->payload);
	}
	else
	{
#endif
		/* broadcast tracking information */
		if(onGround == false)
		{
			frm->type = FRM_TYPE_TRACKING;
			frm->payload_length = serializeTracking(frm->payload);
		}
		else
		{
			frm->type = FRM_TYPE_GROUNDTRACKING;
			frm->payload_length = serializeGroundTracking(frm->payload);
		}
#ifdef FANET_NAME_AUTOBRDCAST
	}
	framecount++;
#endif

	/* in case of a busy channel, ensure that frames from the fifo gets also a change */
	next_tx = HAL_GetTick() + APP_TYPE1OR7_MINTAU_MS;

	return frm;
}


App app = App();

