/*
 * boundingbox.h
 *
 *  Created on: 4 May 2019
 *      Author: sid
 */

#ifndef VARIO_PHY_BOUNDINGBOX_H_
#define VARIO_PHY_BOUNDINGBOX_H_

#include "common.h"

#include "coordinate.h"

class BoundingBox
{
public:
	Coordinate2D min;
	Coordinate2D max;

	BoundingBox(bool initInvExtrema = false) : min(Coordinate2D(initInvExtrema?M_PI_2_f:0.0f, initInvExtrema?M_PI_f:0.0f)),
			max(Coordinate2D(initInvExtrema?-M_PI_2_f:0.0f, initInvExtrema?-M_PI_f:0.0f)) { }
	BoundingBox(const Coordinate2D &poi, float radius) { set(poi, radius, radius); }
	BoundingBox(float topLat, float btmLat, float rightLon, float leftLon) : min(Coordinate2D(btmLat, leftLon)), max(Coordinate2D(topLat, rightLon)) { }

	/* Does the given shape fit within the indicated extents?  */
	bool contained(const BoundingBox &bb);

	/* Do the given boxes overlap at all? */
	bool overlap(const BoundingBox &bb) const;

	bool isInside(const Coordinate2D &poi) const;

	/* Manipulate boundaries */
	void extend(const Coordinate2D &poi);
	void extend(const BoundingBox &bb);
	void set(const Coordinate2D &poi, float latDist_m, float lonDist_m);
	void scale(float factor);

	/* statistics */
	Coordinate2D center();
	float expenseLat(void);
	float expenseLon_atCenter(void);
};

bool operator!=(const BoundingBox &a, const BoundingBox &b);

#endif /* VARIO_PHY_BOUNDINGBOX_H_ */
