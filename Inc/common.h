/*
 * common.h
 *
 *  Created on: Apr 18, 2018
 *      Author: sid
 */

#ifndef COMMON_H_
#define COMMON_H_

#define M_PI_2_f		1.57079632679489661923f
#define M_PI_f			3.14159265358979323846f
#define M_PI			3.14159265358979323846
#define M_2PI_f			6.28318530718f

#define deg2rad(d)		((d) * M_PI_f / 180.0f)
#define rad2deg(r)		((r) * 180.0f / M_PI_f)

#define m2ft(a)			((a) * 3.28084f)
#define m2nm(a)			((a) * 0.000539957f)
#define m2mi(a)			((a) * 0.0006213711922f)
#define mi2m(a)			((a) * 1609.34f)
#define ft2m(a)			((a) * 0.3048f)
#define nm2km(a)		((a) * 1.852f)
#define nm2m(a)			((a) * 1852.0f)
#define fl2m(a)			((a) * 30.4878f)

#define kmph2miph(a)		((a) * 0.621371f)
#define kmph2mps(a)		((a) * 0.2777778f)
#define kmph2kn(a)		((a) * 0.539957f)
#define mps2kmph(a)		((a) * 3.6f)
#define mps2ftpmin(a)		((a) * 196.85f)
#define ftpmin2mps(a)		((a) * 0.00507999983744f)

//#define sgn(a)				((a)>=0?1:-1)
//#define equalf(a, b, toleranz)		(fabsf((a)-(b)) < (toleranz))
#define roundVal(val, r)	(round((val) / (r)) * (r))

#define WGS84_A_RADIUS 		6378137.0f			//meter equator
#define WGS84_B_RADIUS 		6356752.3141f			//meter poles
#define WGS84_MEAN_RADIUS	6371008.8f			//meter

#define WGS84_A_PERIMETER	(WGS84_A_RADIUS * M_2PI_f)
#define WGS84_B_PERIMETER	(WGS84_B_RADIUS * M_2PI_f)

#ifdef __cplusplus

#include <stdint.h>
#include <stddef.h>

template<typename T, size_t N> constexpr size_t NELEM(T (&)[N]) { return N; }
template<typename T> T sgn(T val) { return (T(0) < val) - (val < T(0)); }

#else

#define NELEM(a)  		(sizeof(a) / sizeof(a)[0])

#endif


#endif /* COMMON_H_ */
