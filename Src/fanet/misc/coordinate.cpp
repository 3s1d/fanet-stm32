/*
 * coordinate.cpp
 *
 *  Created on: Jun 13, 2018
 *      Author: sid
 */

#include <cmath>

#include "common.h"

#include "projection.h"
#include "coordinate.h"

Coordinate3D& Coordinate3D::operator=(const Coordinate2D& other)
{
	latitude = other.latitude;
	longitude = other.longitude;
	//altitude stays untouched
	return *this;
}

Coordinate3D& Coordinate3D::operator=(const Coordinate3D& other)
{
	/* self-assignment check expected */
	if (this != &other)
	{
		latitude = other.latitude;
		longitude = other.longitude;
		altitude = other.altitude;
	}

	return *this;
}

float Coordinate3D::distanceTo(const Coordinate2D &target) const
{
	return (2.0f*asinf(sqrtf( pow((sinf((latitude-target.latitude)/2.0f)), 2.0f) +
			cosf(latitude)*cosf(target.latitude)*pow((sinf((longitude-target.longitude)/2.0f)), 2.0f) )) * WGS84_A_RADIUS);
}

float Coordinate3D::angleTo_NxWy(const Coordinate2D &target)	const	//north aligned, mathematical direction
{
	return atan2f(sinf(longitude-target.longitude)*cosf(target.latitude),
			cosf(latitude)*sinf(target.latitude)-sinf(latitude)*cosf(target.latitude)*cosf(longitude-target.longitude));
}

float Coordinate3D::courseTo(const Coordinate2D &target) const
{
	/* angle to course */
	float course = M_2PI_f - angleTo_NxWy(target);
	if(course >= M_2PI_f)
		course -= M_2PI_f;

	return course;
}

Coordinate2D &Coordinate2D::translate(float heading_rad, float distance_m)
{
	distance_m /= WGS84_A_RADIUS;

	float lat_origin = latitude;
	float lon_origin = longitude;
	latitude = asinf( sinf(lat_origin) * cosf(distance_m) + cosf(lat_origin) * sinf(distance_m) * cosf(heading_rad) );
	longitude = lon_origin + atan2f( sinf(heading_rad) * sinf(distance_m) * cosf(lat_origin),
			cosf(distance_m) - sinf(lat_origin) * sinf(latitude) );
	return *this;
}

Coordinate2D &Coordinate2D::operator=(const Coordinate2D& other)
{
	/* self-assignment check expected */
	if (this != &other)
	{
		latitude = other.latitude;
		longitude = other.longitude;
	}

	return *this;
}

Coordinate2D &Coordinate2D::operator()(float lat, float lon)
{
	latitude = lat;
	longitude = lon;
	return *this;
}

Coordinate2D &Coordinate2D::operator=(const Coordinate3D& other)
{
	latitude = other.latitude;
	longitude = other.longitude;
	return *this;
}

float Coordinate2D::distanceTo(const Coordinate2D &target) const
{
	return (2.0f*asinf(sqrtf( pow((sinf((latitude-target.latitude)/2.0f)), 2.0f) +
			cosf(latitude)*cosf(target.latitude)*pow((sinf((longitude-target.longitude)/2.0f)), 2.0f) )) * WGS84_A_RADIUS);
}

float Coordinate2D::angleTo(const Coordinate2D &target)	const			//north aligned, mathematical direction
{
	return atan2f(sinf(longitude-target.longitude)*cosf(target.latitude),
			cosf(latitude)*sinf(target.latitude)-sinf(latitude)*cosf(target.latitude)*cosf(longitude-target.longitude));
}

float Coordinate2D::courseTo(const Coordinate2D &target) const
{
	/* angle to course */
	float course = M_2PI_f - angleTo(target);
	if(course >= M_2PI_f)
		course -= M_2PI_f;

	return course;
}

bool operator==(const Coordinate3D& lhs, const Coordinate3D& rhs)
{
	return lhs.latitude == rhs.latitude && lhs.longitude == rhs.longitude && lhs.altitude == rhs.altitude;
}

Coordinate2D operator-(const Coordinate2D& lhs, const Coordinate2D& rhs)
{
	return Coordinate2D(lhs.latitude-rhs.latitude, lhs.longitude-rhs.longitude);
}

Coordinate2D operator-(const Coordinate2D& lhs, const Coordinate3D& rhs)
{
	return Coordinate2D(lhs.latitude-rhs.latitude, lhs.longitude-rhs.longitude);
}

Coordinate3D operator-(const Coordinate3D& lhs, const Coordinate3D& rhs)
{
	return Coordinate3D(lhs.latitude-rhs.latitude, lhs.longitude-rhs.longitude, lhs.altitude-rhs.altitude);
}

bool operator!=(const Coordinate2D& lhs, const Coordinate2D& rhs)
{
	return (lhs.latitude!=rhs.latitude) || (lhs.longitude!=rhs.longitude);
}


Coordinate2D Coord::translate(const Coordinate2D &base, float heading_rad, float distance_m)
{
	Coordinate2D coord = base;
	coord.translate(heading_rad, distance_m);

	return coord;
}

#if 0
void Coord::boundaries(const Coordinate2D &poi, float latDist_m, float lonDist_m, Coordinate2D &min, Coordinate2D &max)
{
	/* latitude */
	const float angDistLat = latDist_m / WGS84_B_RADIUS;
	max.latitude = poi.latitude + angDistLat;
	min.latitude = poi.latitude - angDistLat;


	/* longitude incl. projection compensation*/
	const float perimter = Projection::longitudeCorrectionAtLatitude(poi.latitude) * WGS84_A_PERIMETER;
	const float ang_dist_lon_deg = lonDist_m * M_2PI_f / perimter;
	max.longitude = poi.longitude + ang_dist_lon_deg;
	min.longitude = poi.longitude - ang_dist_lon_deg;
}
#endif

void Coord::rad2dms(float ang_rad, int &deg, int &min, float &sec)
{
	float coord = rad2deg(ang_rad);
	deg = (int) coord;
	float absDeg = std::abs(coord);
	min = (int) ((absDeg - std::abs((float)deg)) * 60.0f);
	sec = (((absDeg - std::abs((float)deg)) * 60.0) - min) * 60.0f;

	/* ensure limits */
	if(sec >= 60.0f)
	{
		min += 1;
		sec -= 60.0f;
	}
	if(min >= 60)
	{
		deg += (deg>=0) ? 1 : -1;
		min -= 60;
	}
}

void Coord::rad2dm(float ang_rad, int &deg, float &min)
{
	float coord = rad2deg(ang_rad);
	deg = (int) coord;
	coord = std::abs(coord);
	min = (coord - std::abs((float) deg)) * 60.0f;
}

