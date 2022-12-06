/*
 * vec2d.h
 *
 *  Created on: Aug 8, 2018
 *      Author: sid
 */

#ifndef VARIO_MISC_VEC2D_H_
#define VARIO_MISC_VEC2D_H_

#include <cmath>

#include "common.h"

template <class T> class vec3d;
#include "vec3d.h"

template <class T>
class vec2d
{
public:
	union { T x; T longitude; };
	union { T y; T latitude; };

	vec2d() : x(T(0)), y(T(0)) { }
	vec2d(T x, T y) : x(x), y(y) { }
	vec2d(const vec2d& v) : x(v.x), y(v.y) { }

	vec2d operator+(const vec2d& v) const { return vec2d(x + v.x, y + v.y); }
	vec2d operator+(const vec3d<T>& v) const { return vec2d(x + v.x, y + v.y); }
	vec2d operator-(const vec2d& v) const { return vec2d(x - v.x, y - v.y); }

	vec2d& operator=(const vec2d& v) { x = v.x; y = v.y; return *this; }
	vec2d& operator+=(const vec2d& v) { x += v.x; y += v.y; return *this; }
	vec2d& operator-=(const vec2d& v) { x -= v.x; y -= v.y; return *this; }

	vec2d operator+(float s) { return vec2d(x + s, y + s); }
	vec2d operator-(float s) { return vec2d(x - s, y - s);	}
	vec2d operator*(float s) const { return vec2d(x * s, y * s);	}
	vec2d operator/(float s) { return vec2d(x / s, y / s);	}

	vec2d& operator+=(float s) { x += s; y += s; return *this; }
	vec2d& operator-=(float s) { x -= s; y -= s; return *this; }
	vec2d& operator*=(float s) { x *= s; y *= s; return *this; }
	vec2d& operator/=(float s) { x /= s; y /= s; return *this; }

	void set(T x, T y) { this->x = x; this->y = y; }
	vec2d& rotate(float sine, float cosine)
	{
		float tx = x * cosine - y * sine;
		float ty = x * sine + y * cosine;
		x = T(tx);
		y = T(ty);

		return *this;
	}
	vec2d& rotate(float rad)
	{
		float c = std::cos(rad);
		float s = std::sin(rad);

		return rotate(s, c);
	}
	vec2d& rotate_deg(float deg) { return rotate(deg / 180.0 * M_PI_f); }

	vec2d& normalize()
	{
		if (length() == 0)
			return *this;
		*this *= (1.0 / length());
		return *this;
	}

	float dist(vec2d v) const { vec2d d(v.x - x, v.y - y); return d.length(); }

	float length(void) const { return std::sqrt(x * x + y * y); }
	float angle(void) const { return atan2f(y, x); };

	void truncate(float length)
	{
		float angle = atan2f(y, x);
		x = length * cos(angle);
		y = length * sin(angle);
	}
	vec2d ortho(void) const { return vec2d(y, -x); }


	static float dot(vec2d v1, vec2d v2) {	return v1.x * v2.x + v1.y * v2.y; }
	static float cross(vec2d v1, vec2d v2) { return (v1.x * v2.y) - (v1.y * v2.x); }
	static vec2d course2vec2d(T cog) { return vec2d(std::sin(cog), std::cos(cog)); }
	static vec2d angle2vec2d(T cog) { return vec2d(std::cos(cog), std::sin(cog)); }
};

typedef vec2d<float> vec2df;
typedef vec2d<int16_t> vec2di16;


#endif /* VARIO_MISC_VEC2D_H_ */
