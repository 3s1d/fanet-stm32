/*
 * projection.h
 *
 *  Created on: Aug 8, 2018
 *      Author: sid
 */

#ifndef VARIO_PHY_PROJECTION_H_
#define VARIO_PHY_PROJECTION_H_

#include <cmath>

#include "../misc/vec2d.h"
#include "../misc/vec3d.h"
#include "coordinate.h"

class Projection
{
	float pixelPerRad;
	float lonCorrection;
	Coordinate3D _poi;

public:
	const Coordinate3D &poi;

	Projection() : pixelPerRad(0.0f), lonCorrection(1.0f), _poi(), poi(_poi) { }
	Projection(const Coordinate3D &poi, float meterPerPixel = 1.0f);
	void setZoom(float meterPerPixel);
	void setPoi(const Coordinate3D &poi) { this->_poi = poi; lonCorrection = longitudeCorrectionAtLatitude(_poi.latitude); }

	static float longitudeCorrectionAtLatitude(float lat) { return std::cos(lat); }

	void proj(const Coordinate3D &coord, float &x, float &y)
	{
		/* inline to be faster */
		x = (coord.longitude - _poi.longitude) * lonCorrection * pixelPerRad;
		y = (_poi.latitude - coord.latitude) * pixelPerRad;
	}

	void proj(const Coordinate2D &coord, float &x, float &y)
	{
		/* inline to be faster */
		x = (coord.longitude - _poi.longitude) * lonCorrection * pixelPerRad;
		y = (_poi.latitude - coord.latitude) * pixelPerRad;
	}

	void proj(const Coordinate2D &coord, vec2df &target)
	{
		/* inline to be faster */
		target.x = (coord.longitude - _poi.longitude) * lonCorrection * pixelPerRad;
		target.y = (_poi.latitude - coord.latitude) * pixelPerRad;
	}

	vec2df proj(const Coordinate2D &coord)
	{
		vec2df target;

		/* inline to be faster */
		target.x = (coord.longitude - _poi.longitude) * lonCorrection * pixelPerRad;
		target.y = (_poi.latitude - coord.latitude) * pixelPerRad;

		return target;
	}

	vec3df proj(const Coordinate3D &coord)
	{
		vec3df target;

		/* inline to be faster */
		target.x = (coord.longitude - _poi.longitude) * lonCorrection * pixelPerRad;
		target.y = (_poi.latitude - coord.latitude) * pixelPerRad;
		target.z = coord.altitude - _poi.altitude;

		return target;
	}

	/* inverse projection */
	Coordinate3D invProj(const vec3df &xyz)
	{
		Coordinate3D target;

		/* inline to be faster */
		target.longitude = _poi.longitude;
		if(lonCorrection != 0.0f && pixelPerRad != 0.0f)
			target.longitude += xyz.x / (lonCorrection * pixelPerRad);
		target.latitude = _poi.latitude;
		if(pixelPerRad != 0.0f)
			target.latitude -= xyz.y / pixelPerRad;
		target.altitude = xyz.z + _poi.altitude;

		return target;
	}


	void proj(const float &latitude, const float &longitude, float &x, float &y)
	{
		/* inline to be faster */
		x = (longitude - _poi.longitude) * lonCorrection * pixelPerRad;
		y = (_poi.latitude - latitude) * pixelPerRad;
	}

};


#endif /* VARIO_PHY_PROJECTION_H_ */
