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
#include "base.h"

#ifndef M_PI
#define M_PI  (3.141592653589793)
#endif
#define M_TAU (M_PI+M_PI)

#ifndef M_COLINEAR_SLOP
#define M_COLINEAR_SLOP (0.0001f)
#endif

// easy to use two-dimensional vector with shorthand operators
struct Vec2 {
	float x, y;
	
	Vec2() {}
	Vec2(float ax, float ay) : x(ax), y(ay) {}
	Vec2(SDL_Point p) : x(p.x), y(p.y) {}
	Vec2(vec2f u) : x(u.x()), y(u.y()) {}

	#ifdef BOX2D_H
	Vec2(b2Vec2 v) : x(v.x), y(v.y) {}
	operator b2Vec2() { return b2Vec2(x, y); }
	#endif

	void set(float ax, float ay) { x=ax; y=ay; }
	
	operator vec4f() const { return vec4f(x,y,0,0); }

	float real() const         { return x; }
	float imag() const         { return y; }
	float norm() const         { return x*x + y*y; }
	float manhattan() const    { return fabsf(x)+fabsf(y); }
	float magnitude() const    { return sqrtf(norm()); }
	Vec2 conjugate() const     { return Vec2(x,-y); }
	float radians() const      { return atan2f(y,x); }
	Vec2 reflection() const    { return Vec2(y,x); }
	Vec2 anticlockwise() const { return Vec2(-y, x); }
	Vec2 clockwise() const     { return Vec2(y, -x); }
	Vec2 normalized() const    { return (*this) / magnitude(); }

	Vec2 operator+(Vec2 q) const  { return Vec2(x+q.x, y+q.y); }
	Vec2 operator-(Vec2 q) const  { return Vec2(x-q.x, y-q.y); }
	Vec2 operator*(Vec2 q) const  { return Vec2(x*q.x, y*q.y); }
	Vec2 operator/(Vec2 q) const  { return Vec2(x/q.x, y/q.y); }

	Vec2 operator-() const        { return Vec2(-x, -y); }
	Vec2 operator*(float k) const { return Vec2(k*x, k*y); }
	Vec2 operator/(float k) const { return Vec2(x/k, y/k); }

	Vec2 operator +=(Vec2 u)  { x+=u.x; y+=u.y; return *this; }
	Vec2 operator -=(Vec2 u)  { x-=u.x; y-=u.y; return *this; }
	Vec2 operator *=(Vec2 u)  { x*=u.x; y*=u.y; return *this; }
	Vec2 operator /=(Vec2 u)  { x/=u.x; y/=u.y; return *this; }

	Vec2 operator *=(float k) { x*=k; y*=k; return *this; }
	Vec2 operator /=(float k) { x/=k; y/=k; return *this; }
};

// I hate all those 2s cluttering up math code :P
inline Vec2 vec(float ax, float ay) { return Vec2(ax, ay); }

// Vec2 helpers
inline Vec2 operator*(float k, Vec2 q) { return Vec2(k*q.x, k*q.y); }
inline Vec2 operator*(double k, Vec2 q) { return Vec2(k*q.x, k*q.y); }
inline float dot(Vec2 u, Vec2 v) { return u.x*v.x + u.y*v.y; }
inline float cross(Vec2 u, Vec2 v) { return u.x * v.y - v.x* u.y; }
inline Vec2 lerp(Vec2 u, Vec2 v, float t) { return u + t * (v - u); }
inline Vec2 slerp(Vec2 u, Vec2 v, float t) {
	float theta = acosf(dot(u,v));
	float s = 1.f/sinf(theta);
	return (sinf((1-t)*theta)*s)*u + (sinf(t*theta)*s)*v;
}

// simple 2d affine transform
struct AffineMatrix {
	union {
		struct {
			// mathbooks use this stupid a-b-c row-major notation :P
			float a,d,b,e,c,f;
		};
		struct {
			// 
			Vec2 u, v, t;
		};
	};
	

	AffineMatrix() {}
	AffineMatrix(Vec2 au, Vec2 av, Vec2 at) : u(au), v(av), t(at) {}	

	inline AffineMatrix operator *(const AffineMatrix& m) const {
		return AffineMatrix(
			vec(u.x*m.u.x + v.x*m.u.y, 
			    u.y*m.u.x + v.y*m.u.y),
			vec(u.x*m.v.x + v.x*m.v.y, 
			    u.y*m.v.x + v.y*m.v.y),
			vec(u.x*m.t.x + v.x*m.t.y + t.x, 
			    u.y*m.t.x + v.y*m.t.y + t.y)
		);
	}

	inline AffineMatrix operator *=(const AffineMatrix &m) {
		return *this = (*this) * m;
	}

	inline Vec2 transformPoint(const Vec2 p) const {
		return vec(u.x*p.x + v.x*p.y + t.x,
		           u.y*p.x + v.y*p.y + t.y);
	}

	inline Vec2 transformVector(const Vec2 w) const {
		return vec(u.x*w.x + v.x*w.y,
		           u.y*w.x + v.y*w.y);
	}

	float radians() const { return atan2f(u.y, u.x); }
	Vec2 scale() const { return vec(u.magnitude(), v.magnitude()); }
	
	mat4f matrix() const {
		return mat4f({
			u.x, u.y, 0, 0,
			v.x, v.y, 0, 0,
			0, 0, 1, 0,
			t.x, t.y, 0, 1
		});
	}

	bool orthogonal() const { 
		float d = dot(u,v); 
		return -M_COLINEAR_SLOP < d && d < M_COLINEAR_SLOP; 
	}

	bool normal() const {
		float un = u.norm() - 1;
		float vn = v.norm() - 1;
		return un > -M_COLINEAR_SLOP && vn > -M_COLINEAR_SLOP &&
		       un < M_COLINEAR_SLOP && vn < M_COLINEAR_SLOP;
	}

	float determinant() const { return a*e-b*d; }

	AffineMatrix inverse() const {
		float invDet = 1.0f / determinant();
		return AffineMatrix(
			invDet * vec( e, -d ),
			invDet * vec( -b, a ),
			vec( b*f - c*e , c*d - a*f )
		);
	}

	void invert() {
		*this = inverse();
	}

	Vec2 invRigidTransformVector(const Vec2 &w) const {
		return vec(a * w.x + d * w.y,
			       b * w.x + e * w.y);
	}

	Vec2 invRigidTransformPoint(const Vec2& p) const {
		return invRigidTransformVector(p - t);
	}

};

inline AffineMatrix matIdentity() { return AffineMatrix(vec(1,0), vec(0,1), vec(0,0)); }
inline AffineMatrix matTranslation(Vec2 t) { return AffineMatrix(vec(1,0), vec(0,1), t); }
inline AffineMatrix matTranslation(float x, float y) { return matTranslation(vec(x,y)); }
inline AffineMatrix matAttitude(Vec2 dir) { return AffineMatrix(dir, vec(-dir.y,dir.x), vec(0,0)); }
inline AffineMatrix matAttitude(float x, float y) { return matAttitude(vec(x,y)); }
inline AffineMatrix matRotation(float radians) { return matAttitude(cosf(radians), sinf(radians)); }
inline AffineMatrix matPolar(float r, float radians) { return matAttitude(r*cosf(radians), r*sinf(radians)); }
inline AffineMatrix matScale(Vec2 s) { return AffineMatrix(vec(s.x,0), vec(0,s.y), vec(0,0)); }
inline AffineMatrix matScale(float x, float y) { return matScale(vec(x,y)); }
inline AffineMatrix matScale(float k) { return matScale(vec(k,k)); }
inline AffineMatrix matForeshortened(const mat4f& matrix) {
	float buf[16];
	matrix.store(buf);
	return AffineMatrix(
		vec(buf[0], buf[1]), 
		vec(buf[4], buf[5]), 
		vec(buf[12], buf[13])
	);
}

// linear range methods
inline float clamp(float u, float lo=0.f, float hi=1.f) { return u<lo?lo:u>hi?hi:u; }
inline float lerp(float u, float v, float t) { return u + t * (v-u); }
inline float inverseLerp(float u, float v, float t) { return (t-u) / (v-u); }

inline double clampd(double u, double lo=0.f, double hi=1.f) { return u<lo?lo:u>hi?hi:u; }
inline double lerpd(double u, double v, double t) { return u + t * (v-u); }
inline double inverseLerpd(double u, double v, double t) { return (t-u) / (v-u); }

// complex multiplication
inline Vec2  cmul(Vec2  u, Vec2  v) { return Vec2 (u.x*v.x-u.y*v.y, u.x*v.y+u.y*v.x); }

// complex division
inline Vec2 cdiv(Vec2 u, Vec2 v) {
	float normInv = 1.0f/v.norm();
	return Vec2((u.x*v.x+u.y*v.y)*normInv, (v.x*u.y-u.x*v.y)*normInv);
}

// polar -> linear conversion

inline Vec2 polarVector(float radius, float radians) { return radius * Vec2(cosf(radians), sinf(radians)); }
inline Vec2 unitVector(float radians) { return Vec2(cosf(radians), sinf(radians)); }

inline int floorToInt(float x) { return floorf(x); }

// easing functions
inline float easeOut2(float u) {
	u=1.0f-u;
	return 1.0 - u*u;
}
inline float easeOut4(float u) {
	u=1.0f-u;
	return 1.0f - u*u*u*u;
}
inline float easeInOutBack(float t) {
	auto v = t + t;
	auto s = 1.70158 * 1.525;
	if (v < 1.0) {
		return 0.5 * (v * v * ((s + 1.0) * v - s));
	} else {
		v -= 2.0;
		return 0.5 * (v * v * ((s + 1.0) * v + s) + 2.0);
	}
}
inline float easeInOutQuad(float t) { return t<0.5f ? 2.0f*t*t : -1.0f+(4.0f-t-t)*t; }
inline float easeOutBack(float t) { t-=1.0; return t*t*((1.70158+1.0)*t + 1.70158) + 1.0; }

inline float timeIndependentEasing(float easing, float dt) { return 1.0f - powf(1.0f-easing, 60.0f * dt); }
inline float easeTowards(float curr, float target, float easing, float dt) { return curr + (target - curr) * timeIndependentEasing(easing, dt); }
inline Vec2 easeTowards(Vec2 curr, Vec2 target, float easing, float dt)    { return curr + (target - curr) * timeIndependentEasing(easing, dt); }


// random number functions
inline int randInt(int x) { return rand() % x; }
inline int randInt(int inclusiveMin, int exclusiveMax) { return inclusiveMin + randInt(exclusiveMax-inclusiveMin); }
inline float randomValue() { return rand() / double(RAND_MAX); }
inline float randomValue(float u, float v) { return u + randomValue() * (v - u); }
inline float expovariate(float avgDuration, float rmin=0.00001f, float rmax=0.99999f) { return -avgDuration*log(randomValue(rmin, rmax)); }
	
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
	return curr + powf(easing, clampd(1.0/(60.0*dt))) * radianDiff(target, curr);
}

// I USE THIS A LOT O____O
inline float parabola(float x) {
	x = 1 - x - x;
	return 1.0f - x * x;
}

// // line-line intersection test
bool linearIntersection(Vec2 u0, Vec2 u1, Vec2 v0, Vec2 v1, float& u);
bool linearIntersection(Vec2 u0, Vec2 u1, Vec2 v0, Vec2 v1, float& u, float& v);

// // some common curves
Vec2 quadraticBezier(Vec2 p0, Vec2 p1, Vec2 p2, float u);
Vec2 quadraticBezierDeriv(Vec2 p0, Vec2 p1, Vec2 p2, float u);
Vec2 cubicBezier(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float u);
Vec2 cubicBezierDeriv(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float u);
Vec2 cubicHermite(Vec2 p0, Vec2 m0, Vec2 p1, Vec2 m1, float u);
Vec2 cubicHermiteDeriv(Vec2 p0, Vec2 m0, Vec2 p1, Vec2 m1, float u);

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

inline Color easeTowards(Color curr, Color target, float easing, float dt) {
	return lerp(curr, target, timeIndependentEasing(easing, dt));
}


