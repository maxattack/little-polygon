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
struct lpVec {
	lpFloat x, y;
	
	lpVec() {}
	lpVec(lpFloat ax, lpFloat ay) : x(ax), y(ay) {}
	lpVec(SDL_Point p) : x((lpFloat)p.x), y((lpFloat)p.y) {}

	#ifdef BOX2D_H
	lpVec(b2Vec2 v) : x(v.x), y(v.y) {}
	operator b2Vec2() { return b2Vec2(x, y); }
	#endif

	void set(lpFloat ax, lpFloat ay) { x=ax; y=ay; }

	lpFloat real() const         { return x; }
	lpFloat imag() const         { return y; }
	lpFloat norm() const         { return x*x + y*y; }
	lpFloat manhattan() const    { return lpAbs(x)+lpAbs(y); }
	lpFloat magnitude() const    { return lpSqrt(norm()); }
	lpVec conjugate() const     { return lpVec(x,-y); }
	lpFloat radians() const      { return lpAtan2(y,x); }
	lpVec reflection() const    { return lpVec(y,x); }
	lpVec anticlockwise() const { return lpVec(-y, x); }
	lpVec clockwise() const     { return lpVec(y, -x); }
	lpVec normalized() const    { return (*this) / magnitude(); }

	lpVec operator+(lpVec q) const  { return lpVec(x+q.x, y+q.y); }
	lpVec operator-(lpVec q) const  { return lpVec(x-q.x, y-q.y); }
	lpVec operator*(lpVec q) const  { return lpVec(x*q.x, y*q.y); }
	lpVec operator/(lpVec q) const  { return lpVec(x/q.x, y/q.y); }

	lpVec operator-() const        { return lpVec(-x, -y); }
	lpVec operator*(lpFloat k) const { return lpVec(k*x, k*y); }
	lpVec operator/(lpFloat k) const { return lpVec(x/k, y/k); }

	lpVec operator +=(lpVec u)  { x+=u.x; y+=u.y; return *this; }
	lpVec operator -=(lpVec u)  { x-=u.x; y-=u.y; return *this; }
	lpVec operator *=(lpVec u)  { x*=u.x; y*=u.y; return *this; }
	lpVec operator /=(lpVec u)  { x/=u.x; y/=u.y; return *this; }

	lpVec operator *=(lpFloat k) { x*=k; y*=k; return *this; }
	lpVec operator /=(lpFloat k) { x/=k; y/=k; return *this; }
};

// I hate all those 2s cluttering up math code :P
inline lpVec vec(lpFloat ax, lpFloat ay) { return lpVec(ax, ay); }

// lpVec helpers
inline lpVec operator*(lpFloat k, lpVec q) { return lpVec(k*q.x, k*q.y); }
inline lpVec operator*(double k, lpVec q) { return lpVec((lpFloat)k*q.x, (lpFloat)k*q.y); }
inline lpFloat dot(lpVec u, lpVec v) { return u.x*v.x + u.y*v.y; }
inline lpFloat cross(lpVec u, lpVec v) { return u.x * v.y - v.x* u.y; }
inline lpVec lerp(lpVec u, lpVec v, lpFloat t) { return u + t * (v - u); }
inline lpVec slerp(lpVec u, lpVec v, lpFloat t)
{
	lpFloat theta = lpCos(dot(u,v));
	lpFloat s = 1.f / lpSin(theta);
	return (lpSin((1-t)*theta)*s)*u + (lpSin(t*theta)*s)*v;
}

//--------------------------------------------------------------------------------
// AFFINE MATRIX

// simple 2d affine transform
struct lpMatrix {
	union {
		struct {
			// mathbooks use this stupid a-b-c row-major notation :P
			lpFloat a,d,b,e,c,f;
		};
		struct {
			// 
			lpVec u, v, t;
		};
	};
	

	lpMatrix() {}
	lpMatrix(lpVec au, lpVec av, lpVec at) : u(au), v(av), t(at) {}	

	inline lpMatrix operator *(const lpMatrix& m) const {
		return lpMatrix(
			vec(u.x*m.u.x + v.x*m.u.y, 
			    u.y*m.u.x + v.y*m.u.y),
			vec(u.x*m.v.x + v.x*m.v.y, 
			    u.y*m.v.x + v.y*m.v.y),
			vec(u.x*m.t.x + v.x*m.t.y + t.x, 
			    u.y*m.t.x + v.y*m.t.y + t.y)
		);
	}

	inline lpMatrix operator *=(const lpMatrix &m) {
		return *this = (*this) * m;
	}

	inline lpVec transformPoint(const lpVec p) const {
		return vec(u.x*p.x + v.x*p.y + t.x,
		           u.y*p.x + v.y*p.y + t.y);
	}

	inline lpVec transformVector(const lpVec w) const {
		return vec(u.x*w.x + v.x*w.y,
		           u.y*w.x + v.y*w.y);
	}

	lpFloat radians() const { return lpAtan2(u.y, u.x); }
	lpVec scale() const { return vec(u.magnitude(), v.magnitude()); }
	
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

	lpMatrix inverse() const {
		lpFloat invDet = 1.0f / determinant();
		return lpMatrix(
			invDet * vec( e, -d ),
			invDet * vec( -b, a ),
			vec( b*f - c*e , c*d - a*f )
		);
	}

	void invert() {
		*this = inverse();
	}

	lpVec invRigidTransformVector(const lpVec &w) const {
		return vec(a * w.x + d * w.y,
			       b * w.x + e * w.y);
	}

	lpVec invRigidTransformPoint(const lpVec& p) const {
		return invRigidTransformVector(p - t);
	}

};

inline lpMatrix matIdentity() { return lpMatrix(vec(1,0), vec(0,1), vec(0,0)); }
inline lpMatrix matTranslation(lpVec t) { return lpMatrix(vec(1,0), vec(0,1), t); }
inline lpMatrix matTranslation(lpFloat x, lpFloat y) { return matTranslation(vec(x,y)); }
inline lpMatrix matAttitude(lpVec dir) { return lpMatrix(dir, vec(-dir.y,dir.x), vec(0,0)); }
inline lpMatrix matAttitude(lpFloat x, lpFloat y) { return matAttitude(vec(x,y)); }
inline lpMatrix matRotation(lpFloat radians) { return matAttitude(lpCos(radians), lpSin(radians)); }
inline lpMatrix matPolar(lpFloat r, lpFloat radians) { return matAttitude(r*lpCos(radians), r*lpSin(radians)); }
inline lpMatrix matScale(lpVec s) { return lpMatrix(vec(s.x,0), vec(0,s.y), vec(0,0)); }
inline lpMatrix matScale(lpFloat x, lpFloat y) { return matScale(vec(x,y)); }
inline lpMatrix matScale(lpFloat k) { return matScale(vec(k,k)); }

inline lpMatrix matAttitudeTranslation(lpVec dir, lpVec pos) { return lpMatrix(dir, vec(-dir.y, dir.x), pos); }

//--------------------------------------------------------------------------------
// MISC FUNCTIONS


// linear range methods
inline lpFloat clamp(lpFloat u, lpFloat lo=0.f, lpFloat hi=1.f) { return u<lo?lo:u>hi?hi:u; }
inline lpFloat lerp(lpFloat u, lpFloat v, lpFloat t) { return u + t * (v-u); }
inline lpFloat inverseLerp(lpFloat u, lpFloat v, lpFloat t) { return (t-u) / (v-u); }

// complex multiplication
inline lpVec  cmul(lpVec  u, lpVec  v) { return lpVec (u.x*v.x-u.y*v.y, u.x*v.y+u.y*v.x); }

// complex division
inline lpVec cdiv(lpVec u, lpVec v)
{
	lpFloat normInv = 1.0f/v.norm();
	return lpVec((u.x*v.x+u.y*v.y)*normInv, (v.x*u.y-u.x*v.y)*normInv);
}

// polar -> linear conversion

inline lpVec polarVector(lpFloat radius, lpFloat radians) { return radius * lpVec(lpCos(radians), lpSin(radians)); }
inline lpVec unitVector(lpFloat radians) { return lpVec(lpCos(radians), lpSin(radians)); }

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

inline lpFloat easeIn2(lpFloat u) { return u*u; }
inline lpFloat easeIn4(lpFloat u) { return u*u*u*u; }

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
inline lpVec easeTowards(lpVec curr, lpVec target, lpFloat easing, lpFloat dt)    { return curr + (target - curr) * timeIndependentEasing(easing, dt); }


// random number functions
inline int randInt(int x) { return rand() % x; }
inline int randInt(int inclusiveMin, int exclusiveMax) { return inclusiveMin + randInt(exclusiveMax-inclusiveMin); }
inline lpFloat randomValue() { return (lpFloat)(rand() / double(RAND_MAX)); }
inline lpFloat randomValue(lpFloat u, lpFloat v) { return u + randomValue() * (v - u); }
inline lpFloat randomAngle() { return kTAU * randomValue(); }
inline lpVec randomPointOnCircle(lpFloat r=1.0f) { return polarVector(r, randomAngle()); }
inline lpVec randomPointInsideCircle(lpFloat r=1.0f) { return polarVector(r * randomValue(), randomAngle()); }
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
	return curr + timeIndependentEasing(easing, dt) * radianDiff(target, curr);
}

// I USE THIS A LOT O____O
inline lpFloat parabola(lpFloat x) {
	x = 1 - x - x;
	return 1.0f - x * x;
}

// // line-line intersection test
bool linearIntersection(lpVec u0, lpVec u1, lpVec v0, lpVec v1, lpFloat& u);
bool linearIntersection(lpVec u0, lpVec u1, lpVec v0, lpVec v1, lpFloat& u, lpFloat& v);

// // some common curves
lpVec quadraticBezier(lpVec p0, lpVec p1, lpVec p2, lpFloat u);
lpVec quadraticBezierDeriv(lpVec p0, lpVec p1, lpVec p2, lpFloat u);
lpVec cubicBezier(lpVec p0, lpVec p1, lpVec p2, lpVec p3, lpFloat u);
lpVec cubicBezierDeriv(lpVec p0, lpVec p1, lpVec p2, lpVec p3, lpFloat u);
lpVec cubicHermite(lpVec p0, lpVec m0, lpVec p1, lpVec m1, lpFloat u);
lpVec cubicHermiteDeriv(lpVec p0, lpVec m0, lpVec p1, lpVec m1, lpFloat u);

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


