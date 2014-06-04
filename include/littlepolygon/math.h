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

#define kPI ((lpFloat)M_PI)
#define kTAU ((lpFloat)M_TAU)

#define RAD2DEG(rad) (180.0f*(rad)/kPI)
#define DEG2RAD(deg) (kPI*(deg)/180.0f)

#ifndef M_COLINEAR_SLOP
#define M_COLINEAR_SLOP (0.0001f)
#endif

//--------------------------------------------------------------------------------
// VEC2

// easy to use two-dimensional vector with shorthand operators
struct Vec2 {
	lpFloat x, y;
	
	Vec2() {}
	Vec2(lpFloat ax, lpFloat ay) : x(ax), y(ay) {}
	Vec2(SDL_Point p) : x((lpFloat)p.x), y((lpFloat)p.y) {}

	#ifdef BOX2D_H
	Vec2(b2Vec2 v) : x(v.x), y(v.y) {}
	operator b2Vec2() { return b2Vec2(x, y); }
	#endif

	void set(lpFloat ax, lpFloat ay) { x=ax; y=ay; }

	lpFloat real() const         { return x; }
	lpFloat imag() const         { return y; }
	lpFloat norm() const         { return x*x + y*y; }
	lpFloat manhattan() const    { return lpAbs(x)+lpAbs(y); }
	lpFloat magnitude() const    { return lpSqrt(norm()); }
	Vec2 conjugate() const     { return Vec2(x,-y); }
	lpFloat radians() const      { return lpAtan2(y,x); }
	Vec2 reflection() const    { return Vec2(y,x); }
	Vec2 anticlockwise() const { return Vec2(-y, x); }
	Vec2 clockwise() const     { return Vec2(y, -x); }
	Vec2 normalized() const    { return (*this) / magnitude(); }

	Vec2 operator+(Vec2 q) const  { return Vec2(x+q.x, y+q.y); }
	Vec2 operator-(Vec2 q) const  { return Vec2(x-q.x, y-q.y); }
	Vec2 operator*(Vec2 q) const  { return Vec2(x*q.x, y*q.y); }
	Vec2 operator/(Vec2 q) const  { return Vec2(x/q.x, y/q.y); }

	Vec2 operator-() const        { return Vec2(-x, -y); }
	Vec2 operator*(lpFloat k) const { return Vec2(k*x, k*y); }
	Vec2 operator/(lpFloat k) const { return Vec2(x/k, y/k); }

	Vec2 operator +=(Vec2 u)  { x+=u.x; y+=u.y; return *this; }
	Vec2 operator -=(Vec2 u)  { x-=u.x; y-=u.y; return *this; }
	Vec2 operator *=(Vec2 u)  { x*=u.x; y*=u.y; return *this; }
	Vec2 operator /=(Vec2 u)  { x/=u.x; y/=u.y; return *this; }

	Vec2 operator *=(lpFloat k) { x*=k; y*=k; return *this; }
	Vec2 operator /=(lpFloat k) { x/=k; y/=k; return *this; }
};

// I hate all those 2s cluttering up math code :P
inline Vec2 vec(lpFloat ax, lpFloat ay) { return Vec2(ax, ay); }

// Vec2 helpers
inline Vec2 operator*(lpFloat k, Vec2 q) { return Vec2(k*q.x, k*q.y); }
inline Vec2 operator*(double k, Vec2 q) { return Vec2((lpFloat)k*q.x, (lpFloat)k*q.y); }
inline lpFloat dot(Vec2 u, Vec2 v) { return u.x*v.x + u.y*v.y; }
inline lpFloat cross(Vec2 u, Vec2 v) { return u.x * v.y - v.x* u.y; }
inline Vec2 lerp(Vec2 u, Vec2 v, lpFloat t) { return u + t * (v - u); }
inline Vec2 slerp(Vec2 u, Vec2 v, lpFloat t)
{
	lpFloat theta = lpCos(dot(u,v));
	lpFloat s = 1.f / lpSin(theta);
	return (sinf((1-t)*theta)*s)*u + (sinf(t*theta)*s)*v;
}

//--------------------------------------------------------------------------------
// AFFINE MATRIX

// simple 2d affine transform
struct AffineMatrix {
	union {
		struct {
			// mathbooks use this stupid a-b-c row-major notation :P
			lpFloat a,d,b,e,c,f;
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

	lpFloat radians() const { return atan2f(u.y, u.x); }
	Vec2 scale() const { return vec(u.magnitude(), v.magnitude()); }
	
	bool orthogonal() const { 
		lpFloat d = dot(u,v); 
		return -M_COLINEAR_SLOP < d && d < M_COLINEAR_SLOP; 
	}

	bool normal() const {
		lpFloat un = u.norm() - 1;
		lpFloat vn = v.norm() - 1;
		return un > -M_COLINEAR_SLOP && vn > -M_COLINEAR_SLOP &&
		       un < M_COLINEAR_SLOP && vn < M_COLINEAR_SLOP;
	}

	lpFloat determinant() const { return a*e-b*d; }

	AffineMatrix inverse() const {
		lpFloat invDet = 1.0f / determinant();
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
inline AffineMatrix matTranslation(lpFloat x, lpFloat y) { return matTranslation(vec(x,y)); }
inline AffineMatrix matAttitude(Vec2 dir) { return AffineMatrix(dir, vec(-dir.y,dir.x), vec(0,0)); }
inline AffineMatrix matAttitude(lpFloat x, lpFloat y) { return matAttitude(vec(x,y)); }
inline AffineMatrix matRotation(lpFloat radians) { return matAttitude(lpCos(radians), lpSin(radians)); }
inline AffineMatrix matPolar(lpFloat r, lpFloat radians) { return matAttitude(r*lpCos(radians), r*lpSin(radians)); }
inline AffineMatrix matScale(Vec2 s) { return AffineMatrix(vec(s.x,0), vec(0,s.y), vec(0,0)); }
inline AffineMatrix matScale(lpFloat x, lpFloat y) { return matScale(vec(x,y)); }
inline AffineMatrix matScale(lpFloat k) { return matScale(vec(k,k)); }

inline AffineMatrix matAttitudeTranslation(Vec2 dir, Vec2 pos) { return AffineMatrix(dir, vec(-dir.y, dir.x), pos); }

//--------------------------------------------------------------------------------
// MISC FUNCTIONS


// linear range methods
inline lpFloat clamp(lpFloat u, lpFloat lo=0.f, lpFloat hi=1.f) { return u<lo?lo:u>hi?hi:u; }
inline lpFloat lerp(lpFloat u, lpFloat v, lpFloat t) { return u + t * (v-u); }
inline lpFloat inverseLerp(lpFloat u, lpFloat v, lpFloat t) { return (t-u) / (v-u); }

// complex multiplication
inline Vec2  cmul(Vec2  u, Vec2  v) { return Vec2 (u.x*v.x-u.y*v.y, u.x*v.y+u.y*v.x); }

// complex division
inline Vec2 cdiv(Vec2 u, Vec2 v)
{
	lpFloat normInv = 1.0f/v.norm();
	return Vec2((u.x*v.x+u.y*v.y)*normInv, (v.x*u.y-u.x*v.y)*normInv);
}

// polar -> linear conversion

inline Vec2 polarVector(lpFloat radius, lpFloat radians) { return radius * Vec2(lpCos(radians), lpSin(radians)); }
inline Vec2 unitVector(lpFloat radians) { return Vec2(lpCos(radians), lpSin(radians)); }

inline int floorToInt(lpFloat x) { return (int) lpFloor(x); }

// easing functions
inline lpFloat easeOut2(lpFloat u)
{
	u=1.0f-u;
	return 1.0f - u*u;
}

inline lpFloat easeOut4(lpFloat u)
{
	u=1.0f-u;
	return 1.0f - u*u*u*u;
}

inline lpFloat easeInOutBack(lpFloat t)
{
	auto v = t + t;
	auto s = 1.70158f * 1.525f;
	if (v < 1.0) {
		return 0.5f * (v * v * ((s + 1.0f) * v - s));
	} else {
		v -= 2.0;
		return 0.5f * (v * v * ((s + 1.0f) * v + s) + 2.0f);
	}
}
inline lpFloat easeInOutQuad(lpFloat t) { return t<0.5f ? 2.0f*t*t : -1.0f+(4.0f-t-t)*t; }
inline lpFloat easeOutBack(lpFloat t) { t-=1.0; return t*t*((1.70158f+1.0f)*t + 1.70158f) + 1.0f; }

inline lpFloat timeIndependentEasing(lpFloat easing, lpFloat dt) { return 1.0f - lpPow(1.0f-easing, 60.0f * dt); }
inline lpFloat easeTowards(lpFloat curr, lpFloat target, lpFloat easing, lpFloat dt) { return curr + (target - curr) * timeIndependentEasing(easing, dt); }
inline Vec2 easeTowards(Vec2 curr, Vec2 target, lpFloat easing, lpFloat dt)    { return curr + (target - curr) * timeIndependentEasing(easing, dt); }


// random number functions
inline int randInt(int x) { return rand() % x; }
inline int randInt(int inclusiveMin, int exclusiveMax) { return inclusiveMin + randInt(exclusiveMax-inclusiveMin); }
inline lpFloat randomValue() { return (lpFloat)(rand() / double(RAND_MAX)); }
inline lpFloat randomValue(lpFloat u, lpFloat v) { return u + randomValue() * (v - u); }
inline lpFloat randomAngle() { return kTAU * randomValue(); }
inline Vec2 randomPointOnCircle(lpFloat r=1.0f) { return polarVector(r, randomAngle()); }
inline Vec2 randomPointInsideCircle(lpFloat r=1.0f) { return polarVector(r * randomValue(), randomAngle()); }
inline lpFloat expovariate(lpFloat avgDuration, lpFloat rmin=0.00001f, lpFloat rmax=0.99999f) { return -avgDuration*lpLog(randomValue(rmin, rmax)); }
	
// handling radians sanely
inline lpFloat normalizeAngle(lpFloat radians)
{
	radians = lpMod(radians, kTAU);
	return radians < 0 ? radians + kTAU : radians;
}

inline lpFloat radianDiff(lpFloat lhs, lpFloat rhs)
{
	lpFloat result = normalizeAngle(lhs - rhs);
	if (result > (lpFloat) M_PI) {
		return result - kTAU;
	} else if (result < -kPI) {
		return result + kTAU;
	} else {
		return result;
	}
}

inline lpFloat lerpRadians(lpFloat a0, lpFloat a1, lpFloat t)
{
	return a0 + t * radianDiff(a1, a0);
}

inline lpFloat easeRadians(lpFloat curr, lpFloat target, lpFloat easing, lpFloat dt)
{
	return curr + powf(easing, clamp(1.0f/(60.0f*dt))) * radianDiff(target, curr);
}

// I USE THIS A LOT O____O
inline lpFloat parabola(lpFloat x) {
	x = 1 - x - x;
	return 1.0f - x * x;
}

// // line-line intersection test
bool linearIntersection(Vec2 u0, Vec2 u1, Vec2 v0, Vec2 v1, lpFloat& u);
bool linearIntersection(Vec2 u0, Vec2 u1, Vec2 v0, Vec2 v1, lpFloat& u, lpFloat& v);

// // some common curves
Vec2 quadraticBezier(Vec2 p0, Vec2 p1, Vec2 p2, lpFloat u);
Vec2 quadraticBezierDeriv(Vec2 p0, Vec2 p1, Vec2 p2, lpFloat u);
Vec2 cubicBezier(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, lpFloat u);
Vec2 cubicBezierDeriv(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, lpFloat u);
Vec2 cubicHermite(Vec2 p0, Vec2 m0, Vec2 p1, Vec2 m1, lpFloat u);
Vec2 cubicHermiteDeriv(Vec2 p0, Vec2 m0, Vec2 p1, Vec2 m1, lpFloat u);

//--------------------------------------------------------------------------------
// COLOR

struct Color {
	union {
		uint32_t abgr; // big-endian opengl byte order :P
		struct {
			uint8_t r, g, b, a;
		};
	};

	Color() {}
	Color(uint8_t ar, uint8_t ag, uint8_t ab, uint8_t aa=0xff) : r(ar), g(ag), b(ab), a(aa) {}

	#ifdef BOX2D_H
	// helpers if we're using box2D
	Color(b2Color c) : r(0xff*c.r), g(0xff*c.g), b(0xff*c.b), a(0xff) {}
	operator b2Color() { return b2Color(red(), green(), blue()); }
	#endif

	lpFloat red() { return r * (1.f/255.f); }
	lpFloat green() { return g * (1.f/255.f); }
	lpFloat blue() { return b * (1.f/255.f); }
	lpFloat alpha() { return a * (1.f/255.f); }
	
	void setRed(lpFloat ar) { r = (uint8_t) floorToInt(255.0f * ar); }
	void setGreen(lpFloat ag) { g = (uint8_t) floorToInt(255.0f * ag); }
	void setBlue(lpFloat ab) { b = (uint8_t) floorToInt(255.0f * ab); }
	void setAlpha(lpFloat aa) { a = (uint8_t) floorToInt(255.0f * aa); }
	
	void toHSV(lpFloat *h, lpFloat *s, lpFloat *v);
};

// handy verbose constructors
inline Color rgba(uint32_t hex) { return Color( (0xff000000&hex)>>24, (0xff0000&hex)>>16, (0xff00&hex)>>8, 0xff&hex ); }
inline Color rgb(uint32_t hex) { return Color( (0xff0000&hex)>>16, (0xff00&hex)>>8, 0xff&hex, 0xff ); }
inline Color rgb(lpFloat r, lpFloat g, lpFloat b) { return Color((uint8_t)floorToInt(0xff*r), (uint8_t)floorToInt(0xff*g), (uint8_t)floorToInt(0xff*b)); }
inline Color rgba(lpFloat r, lpFloat g, lpFloat b, lpFloat a) { return Color((uint8_t)floorToInt(0xff*r), (uint8_t)floorToInt(0xff*g), (uint8_t)floorToInt(0xff*b), (uint8_t)floorToInt(0xff*a)); }
inline Color rgba(Color c, lpFloat a) { c.a = (uint8_t)floorToInt(0xff * a); return c; }
Color hsv(lpFloat h, lpFloat s, lpFloat v);
inline Color hsva(lpFloat h, lpFloat s, lpFloat v, lpFloat a) { return rgba(hsv(h,s,v), a); }

// interpolate in linear-color space
inline Color lerp(Color a, Color b, lpFloat u)
{
	return rgba(
		lerp(a.red(), b.red(), u),
		lerp(a.green(), b.green(), u),
		lerp(a.blue(), b.blue(), u),
		lerp(a.alpha(), b.alpha(), u)
	);
}

inline Color easeTowards(Color curr, Color target, lpFloat easing, lpFloat dt)
{
	return lerp(curr, target, timeIndependentEasing(easing, dt));
}


