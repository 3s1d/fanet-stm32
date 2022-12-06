/*
 * projection.cpp
 *
 *  Created on: Aug 8, 2018
 *      Author: sid
 */

#include <cmath>

#include "common.h"

#include "projection.h"

Projection::Projection(const Coordinate3D &poi, float meterPerPixel) : poi(_poi)
{
	/* zoom */
	setZoom(meterPerPixel);

	/* poi */
	_poi = poi;

	/* longitude correction */
	lonCorrection = longitudeCorrectionAtLatitude(_poi.latitude);
}

void Projection::setZoom(float meterPerPixel)
{
	if(meterPerPixel == 0.0f)
		pixelPerRad = 0.0f;
	else
		pixelPerRad = WGS84_A_RADIUS / meterPerPixel; //--> ((radius * 2pi) / 2pi) / meterPerPixel
}

