/*
 * vec3d.h
 *
 *  Created on: 31 Dec 2018
 *      Author: sid
 */

#ifndef VARIO_MISC_VEC3D_H_
#define VARIO_MISC_VEC3D_H_

#include <cmath>

template <class T> class vec2d;
#include "vec2d.h"

template <class T>
class vec3d
{
public:
	union { T x; T longitude; };
	union { T y; T latitude; };
	union { T z; T altitude; };

	vec3d operator+(const vec3d& v) const { return vec3d(x + v.x, y + v.y, z + v.z); }
	vec2d<T> operator+(const vec2d<T>& v) const { return vec2d<T>(x + v.x, y + v.y); }
	vec3d operator-(const vec3d& v) const { return vec3d(x - v.x, y - v.y, z - v.z); }

	vec3d operator+(float s) { return vec3d(x + s, y + s, z + s); }
	vec3d operator-(float s) { return vec3d(x - s, y - s, z - s); }
	vec3d operator*(float s) { return vec3d(x * s, y * s, z * s); }
	vec3d operator/(float s) { return vec3d(x / s, y / s, z / s); }

	vec3d() : x(T(0)), y(T(0)), z(T(0)) { }
	vec3d(T x, T y, T z) : x(x), y(y), z(z) { }
	vec3d(const vec3d& v) : x(v.x), y(v.y), z(v.z) { }

	float dist(const vec3d &v) const { vec3d d(v.x - x, v.y - y, v.z - z); return d.length(); }
	float length(void) const { return std::sqrt(x * x + y * y + z * z); }

	vec3d& rotate(float sine, float cosine)
	{
		float tx = x * cosine - y * sine;
		float ty = x * sine + y * cosine;
		x = T(tx);
		y = T(ty);

		return *this;
	}

	static float dot(const vec3d &v1, const vec3d &v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }
	static vec3d pointOnLine(const vec3d &v1, const vec3d &v2, float t) { return v1 + (v2-v1)*t; }
};


typedef vec3d<float> vec3df;

#endif /* VARIO_MISC_VEC3D_H_ */
