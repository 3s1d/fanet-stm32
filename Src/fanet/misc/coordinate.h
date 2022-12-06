/*
 * coordinate.h
 *
 *  Created on: Jun 13, 2018
 *      Author: sid
 */

#ifndef VARIO_PHY_COORDINATE_H_
#define VARIO_PHY_COORDINATE_H_

#include "vec2d.h"

class Coordinate			//pseudo class to intersect 2D/3D
{
public:
	float latitude;
	float longitude;

	Coordinate(void) : latitude(0.0f), longitude(0.0f) { }
	Coordinate(float lat, float lon) : latitude(lat), longitude(lon) { }
};

class Coordinate2D;

class Coordinate3D /*: public Coordinate*/
{
public:
	float latitude;
	float longitude;
	float altitude;

	Coordinate3D(void) : latitude(0.0f), longitude(0.0f), altitude(0.0f) { }
	Coordinate3D(float lat, float lon, float alt=0.0f) : latitude(lat), longitude(lon), altitude(alt) { }
	Coordinate3D(const Coordinate &c) : latitude(c.latitude), longitude(c.longitude), altitude(0.0f) { }

	/* operators */
	Coordinate3D &operator=(const Coordinate2D& other);
	Coordinate3D &operator=(const Coordinate3D& other);

	float distanceTo(const Coordinate2D &target) const;
	float angleTo_NxWy(const Coordinate2D &target) const;
	float courseTo(const Coordinate2D &target) const;
};

class Coordinate2D : public Coordinate
{
public:
	using Coordinate::Coordinate;
	Coordinate2D(const Coordinate3D &c3d) : Coordinate(c3d.latitude, c3d.longitude) { }

	/* operators */
	Coordinate2D &operator=(const Coordinate2D& other);
	Coordinate2D &operator=(const Coordinate3D& other);
	Coordinate2D &translate(float heading_rad, float distance_m);
	Coordinate2D &operator()(float lat, float lon);

	Coordinate2D& operator-=(const Coordinate2D &rhs) { latitude -= rhs.latitude; longitude -= rhs.longitude; return *this; }
	Coordinate2D& operator+=(const Coordinate2D &rhs) { latitude += rhs.latitude; longitude += rhs.longitude; return *this; }
	Coordinate2D& operator+=(const vec2df &rhs) { latitude += rhs.latitude; longitude += rhs.longitude; return *this; }
	Coordinate2D& operator/=(const float scale) { latitude /= scale; longitude /= scale; return *this; }

	float distanceTo(const Coordinate2D &target) const;
	float angleTo(const Coordinate2D &target) const;
	float courseTo(const Coordinate2D &target) const;
};

Coordinate2D operator-(const Coordinate2D& lhs, const Coordinate2D& rhs);
Coordinate2D operator-(const Coordinate2D& lhs, const Coordinate3D& rhs);
Coordinate3D operator-(const Coordinate3D& lhs, const Coordinate3D& rhs);
bool operator!=(const Coordinate2D& lhs, const Coordinate2D& rhs);
bool operator==(const Coordinate3D& lhs, const Coordinate3D& rhs);


namespace Coord
{
	Coordinate2D translate(const Coordinate2D &base, float heading_rad, float distance_m);
//	void boundaries(const Coordinate2D &poi, float latDist_m, float lonDist_m, Coordinate2D &min, Coordinate2D &max);
	void rad2dms(float ang_rad, int &deg, int &min, float &sec);
	void rad2dm(float ang_rad, int &deg, float &min);
}

#endif /* VARIO_PHY_COORDINATE_H_ */
