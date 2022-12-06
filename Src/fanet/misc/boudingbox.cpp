/*
 * boudingbox.cpp
 *
 *  Created on: 4 May 2019
 *      Author: sid
 */

#include "common.h"

#include "projection.h"
#include "boundingbox.h"

void BoundingBox::extend(const Coordinate2D &poi)
{
	if(poi.latitude < min.latitude)
		min.latitude = poi.latitude;
	if(poi.longitude < min.longitude)
		min.longitude = poi.longitude;
	if(poi.latitude > max.latitude)
		max.latitude = poi.latitude;
	if(poi.longitude > max.longitude)
		max.longitude = poi.longitude;
}

void BoundingBox::extend(const BoundingBox &bb)
{
	if(bb.min.longitude < min.longitude)
		min.longitude = bb.min.longitude;
	if(bb.min.latitude < min.latitude)
		min.latitude = bb.min.latitude;
	if(bb.max.longitude > max.longitude)
		max.longitude = bb.max.longitude;
	if(bb.max.latitude > max.latitude)
		max.latitude = bb.max.latitude;
}

void BoundingBox::set(const Coordinate2D &poi, float latDist_m, float lonDist_m)
{
	/* latitude */
	const float angDistLat = latDist_m / WGS84_B_RADIUS;
	max.latitude = poi.latitude + angDistLat;
	min.latitude = poi.latitude - angDistLat;

	/* longitude incl projection compensation*/
	const float perimter = Projection::longitudeCorrectionAtLatitude(poi.latitude) * WGS84_A_PERIMETER;
	const float ang_dist_lon_deg = lonDist_m * M_2PI_f / perimter;
	max.longitude = poi.longitude + ang_dist_lon_deg;
	min.longitude = poi.longitude - ang_dist_lon_deg;
}

void BoundingBox::scale(float factor)
{
	const float latExtend = std::abs(max.latitude - min.latitude) * (1.0f-factor) / 2.0f;
	const float lonExtend = std::abs(max.longitude - min.longitude) * (1.0f-factor) / 2.0f;
	min.latitude -= latExtend;
	min.longitude -= lonExtend;
	max.latitude += latExtend;
	max.longitude += lonExtend;
}

bool BoundingBox::contained(const BoundingBox &bb)
{
	return !(bb.min.longitude < min.longitude || bb.max.longitude > max.longitude || bb.min.latitude < min.latitude || bb.max.latitude > max.latitude);
}

bool BoundingBox::overlap(const BoundingBox &bb) const
{
	if (bb.max.longitude < min.longitude)
		return false;

	if (max.longitude < bb.min.longitude)
		return false;

	if (bb.max.latitude < min.latitude)
		return false;

	if (max.latitude < bb.min.latitude)
		return false;

	return true;
}

bool BoundingBox::isInside(const Coordinate2D &poi) const
{
	return (poi.latitude>=min.latitude && poi.latitude<=max.latitude && poi.longitude>=min.longitude && poi.longitude<=max.longitude);
}

Coordinate2D BoundingBox::center()
{
	return Coordinate2D((max.latitude+min.latitude) / 2.0f, (max.longitude+min.longitude) / 2.0f);
}

float BoundingBox::expenseLat(void)
{
	const float centerLon = (max.longitude+min.longitude) / 2.0f;
	return Coordinate2D(max.latitude, centerLon).distanceTo(Coordinate2D(min.latitude, centerLon));
}

float BoundingBox::expenseLon_atCenter(void)
{
	const float centerLat = (max.latitude+min.latitude) / 2.0f;
	return Coordinate2D(centerLat, max.longitude).distanceTo(Coordinate2D(centerLat, min.longitude));
}


bool operator!=(const BoundingBox &a, const BoundingBox &b)
{
	return (a.min != b.min) || (a.max != b.max);
}
