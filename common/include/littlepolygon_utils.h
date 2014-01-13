// Little Polygon SDK
// Copyright (C) 2013 Max Kaufmann
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once
#include "littlepolygon_base.h"

// A garbage-bin of additional helpful utilities not necessarily required by LPAAT,
// which I've accumulated and rolled over from project to project :P

//--------------------------------------------------------------------------------
// COROUTINE MACROS
//--------------------------------------------------------------------------------

#define COROUTINE_PARAMETER           int _line;
#define COROUTINE_RESET               _line=0;
#define COROUTINE_DISABLE             _line=-1;
#define COROUTINE_ACTIVE              (_line!=-1)
#define COROUTINE_BEGIN               switch(_line){case 0:;
#define COROUTINE_YIELD               {_line=__LINE__; return; case __LINE__:;}
#define COROUTINE_YIELD_RESULT(_x)    {_line=__LINE__;return (_x); case __LINE__:;}
#define COROUTINE_END                 _line=-1;default:;}

//--------------------------------------------------------------------------------
// GEOMETRY UTILITIES
//--------------------------------------------------------------------------------

// handy constants
#ifndef M_PI
#define M_PI  (3.141592653589793)
#endif
#define M_TAU (M_PI+M_PI)

#ifndef M_COLINEAR_SLOP
#define M_COLINEAR_SLOP (0.0001f)
#endif

// easy to use two-dimensional vector with shorthand operators
struct vec2 {
	float x, y;
	
	vec2() {}
	vec2(float ax, float ay) : x(ax), y(ay) {}
	vec2(SDL_Point p) : x(p.x), y(p.y) {}
	vec2(vec2f u) : x(u.x()), y(u.y()) {}

	#ifdef BOX2D_H
	vec2(b2Vec2 v) : x(v.x), y(v.y) {}
	operator b2Vec2() { return b2Vec2(x, y); }
	#endif

	float real() const         { return x; }
	float imag() const         { return y; }
	float norm() const         { return x*x + y*y; }
	float manhattan() const    { return fabs(x)+fabs(y); }
	float magnitude() const    { return sqrtf(norm()); }
	vec2 conjugate() const     { return vec2(x,-y); }
	float radians() const      { return atan2f(y,x); }
	vec2 reflection() const    { return vec2(y,x); }
	vec2 anticlockwise() const { return vec2(-y, x); }
	vec2 clockwise() const     { return vec2(y, -x); }
	vec2 normalized() const    { return (*this) / magnitude(); }

	vec2 operator+(vec2 q) const  { return vec2(x+q.x, y+q.y); }
	vec2 operator-(vec2 q) const  { return vec2(x-q.x, y-q.y); }
	vec2 operator*(vec2 q) const  { return vec2(x*q.x, y*q.y); }
	vec2 operator/(vec2 q) const  { return vec2(x/q.x, y/q.y); }

	vec2 operator-() const        { return vec2(-x, -y); }
	vec2 operator*(float k) const { return vec2(k*x, k*y); }
	vec2 operator/(float k) const { return vec2(x/k, y/k); }

	vec2 operator +=(vec2 u)  { x+=u.x; y+=u.y; return *this; }
	vec2 operator -=(vec2 u)  { x-=u.x; y-=u.y; return *this; }
	vec2 operator *=(vec2 u)  { x*=u.x; y*=u.y; return *this; }
	vec2 operator /=(vec2 u)  { x/=u.x; y/=u.y; return *this; }

	vec2 operator *=(float k) { x*=k; y*=k; return *this; }
	vec2 operator /=(float k) { x/=k; y/=k; return *this; }
};

// I hate all those 2s cluttering up math code :P	
inline vec2 vec(float ax, float ay) { return vec2(ax, ay); }

// vec2 helpers
inline vec2 operator*(float k, vec2 q) { return vec2(k*q.x, k*q.y); }
inline float dot(vec2 u, vec2 v) { return u.x*v.x + u.y*v.y; }
inline float cross(vec2 u, vec2 v) { return u.x * v.y - v.x* u.y; }
inline vec2 lerp(vec2 u, vec2 v, float t) { return u + t * (v - u); }
inline vec2 slerp(vec2 u, vec2 v, float t) {
	float theta = acosf(dot(u,v));
	float s = 1.f/sinf(theta);
	return (sinf((1-t)*theta)*s)*u + (sinf(t*theta)*s)*v;
}

// linear range methods
inline float clamp(float u, float lo=0.f, float hi=1.f) { return u<lo?lo:u>hi?hi:u; }
inline float lerp(float u, float v, float t) { return u + t * (v-u); }

// complex multiplication
inline vec2 cmul(vec2 u, vec2 v) { return vec2(u.x*v.x-u.y*v.y, u.x*v.y+u.y*v.x); }

// complex division
inline vec2 cdiv(vec2 u, vec2 v) {
	float normInv = 1.0f/v.norm();
	return vec2((u.x*v.x+u.y*v.y)*normInv, (v.x*u.y-u.x*v.y)*normInv);
}

// polar -> linear conversion
inline vec2 polar(float radius, float radians) { return radius * vec2(cosf(radians), sinf(radians)); }

// easing functions
inline float easeOut2(float u) {
	u=1.0-u;
	return 1.0 - u*u;
}
inline float easeOut4(float u) {
	u=1.0-u;
	return 1.0 - u*u*u*u;
}
inline float easeInOutBack(float t, float s=1.70158f) { return (s+1)*t*t*t - s*t*t; }
inline float easeTowards(float curr, float target, float easing, float dt) { return curr + powf(easing, clamp(60*dt)) * (target - curr); }
inline vec2 easeTowards(vec2 curr, vec2 target, float easing, float dt) { float k = powf(easing, clamp(60*dt)); return curr + k * (target - curr); }

// random number functions
inline int randInt(int x) { return rand() % x; }
inline double randomValue() { return rand() / double(RAND_MAX); }
inline double randomValue(double u, double v) { return u + randomValue() * (v - u); }
inline double expovariate(double avgDuration) { return -avgDuration * log(1.0 - randomValue(0.0000001, 0.999999)); }
	
// handling radians sanely
inline float normalizeAngle(float radians) {
	radians = fmodf(radians, M_TAU);
	return radians < 0 ? radians + M_TAU : radians;
}
inline float radianDiff(float lhs, float rhs) {
	float result = normalizeAngle(lhs - rhs);
	if (result > M_PI) {
		return result - M_TAU;
	} else if (result < -M_PI) {
		return result + M_TAU;
	} else {
		return result;
	}
}
inline float easeRadians(float curr, float target, float easing, float dt) {
	return curr + powf(easing, clamp(60*dt)) * radianDiff(target, curr);
}

// I USE THIS A LOT O____O
inline float parabola(float x) {
	x = 1 - x - x;
	return 1.0f - x * x;
}

// // line-line intersection test
// bool linearIntersection(vec2 u0, vec2 u1, vec2 v0, vec2 v1, float& u);
// bool linearIntersection(vec2 u0, vec2 u1, vec2 v0, vec2 v1, float& u, float& v);

// // some common curves
// vec2 quadraticBezier(vec2 p0, vec2 p1, vec2 p2, float u);
// vec2 quadraticBezierDeriv(vec2 p0, vec2 p1, vec2 p2, float u);
// vec2 cubicBezier(vec2 p0, vec2 p1, vec2 p2, vec2 p3, float u);
// vec2 cubicBezierDeriv(vec2 p0, vec2 p1, vec2 p2, vec2 p3, float u);
// vec2 cubicHermite(vec2 p0, vec2 m0, vec2 p1, vec2 m1, float u);
// vec2 cubicHermiteDeriv(vec2 p0, vec2 m0, vec2 p1, vec2 m1, float u);

//--------------------------------------------------------------------------------
// COLOR HELPER
//--------------------------------------------------------------------------------

struct Color {
	union {
		uint32_t abgr; // big-endian opengl byte order :P
		struct {
			uint8_t r, g, b, a; // little endian
		};
	};

	Color() {}
	Color(uint8_t ar, uint8_t ag, uint8_t ab, uint8_t aa=0xff) : r(ar), g(ag), b(ab), a(aa) {}

	#ifdef BOX2D_H
	// helpers if we're using box2D
	Color(b2Color c) : r(0xff*c.r), g(0xff*c.g), b(0xff*c.b), a(0xff) {}
	operator b2Color() { return b2Color(red(), green(), blue()); }
	#endif

	float red() { return r * (1.f/255.f); }
	float green() { return g * (1.f/255.f); }
	float blue() { return b * (1.f/255.f); }
	float alpha() { return a * (1.f/255.f); }
	void toHSV(float *h, float *s, float *v);
};

// handy verbose constructors
inline Color rgba(uint32_t hex) { return Color( (0xff000000&hex)>>24, (0xff0000&hex)>>16, (0xff00&hex)>>8, 0xff&hex ); }
inline Color rgb(uint32_t hex) { return Color( (0xff0000&hex)>>16, (0xff00&hex)>>8, 0xff&hex, 0xff ); }
inline Color rgb(float r, float g, float b) { return Color(0xff*r, 0xff*g, 0xff*b); }
inline Color rgba(float r, float g, float b, float a) { return Color(0xff*r, 0xff*g, 0xff*b, 0xff*a); }
inline Color rgba(Color c, float a) { c.a = 0xff * a; return c; }
Color hsv(float h, float s, float v);
inline Color hsva(float h, float s, float v, float a) { return rgba(hsv(h,s,v), a); }

// interpolate in linear-color space
inline Color lerp(Color a, Color b, float u) {
	return rgba(
		lerp(a.red(), b.red(), u),
		lerp(a.green(), b.green(), u),
		lerp(a.blue(), b.blue(), u),
		lerp(a.alpha(), b.alpha(), u)
	);
}

//--------------------------------------------------------------------------------
// TIMING & ANIMATION UTILITIES
//--------------------------------------------------------------------------------

struct Timer {
	// sdl ticks since the timer started
	int ticks;
	int deltaTicks;

	// virtually scaled "seconds" since the timer started
	double timeScale;
	double scaledTime;
	double scaledDeltaTime;

	Timer(float aTimeScale=1) : timeScale(aTimeScale) {
		reset();
	}

	void reset() {
		ticks = SDL_GetTicks();
		deltaTicks = 0;
		scaledTime = 0;
		scaledDeltaTime = 0;
	}

	void skipTicks() {
		// for things, like, coming back from pause
		ticks = SDL_GetTicks();
	}

	void tick() {
		deltaTicks = SDL_GetTicks() - ticks;
		scaledDeltaTime = timeScale * deltaSeconds();
		ticks += deltaTicks;
		scaledTime += scaledDeltaTime;
	}

	double seconds() const { return 0.001 * ticks; }
	double deltaSeconds() const { return 0.001 * deltaTicks; }
};

inline int pingPong(int i, int n) {
	i  = i % (n + n - 2);
	return i >= n ? 2 * (n-1) - i : i;
}	

