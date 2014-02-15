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

#include "littlepolygon_graphics.h"

//------------------------------------------------------------------------------
// SPLINE REDNERING
//------------------------------------------------------------------------------

// This is a handy way to render splines without uploading vertices.  A single
// static vertex batch is buffered with "parametric coordinates" of the form
// <x^3, x^2, x^1, 1> which are "programmed" by a unifrm hermite matrix.  Supports
// Non-uniform tapering as well for making things like tentacles :P

struct SplinePlotter;
class SplinePlotterRef;

SplinePlotterRef createSplinePlotter(int resolution=64);

class SplinePlotterRef {
private:
	SplinePlotter *context;

public:
	SplinePlotterRef() {}
	SplinePlotterRef(SplinePlotter *aContext) : context(aContext) {}

	operator SplinePlotter*() { return context; }
	operator bool() const { return context; }

	void destroy();

	void begin(const Viewport& viewport);
	void plot(mat4f positionMatrix, vec4f strokeVector, Color c);
	void end();
};

class SplinePlotterHandle : public SplinePlotterRef {
public:
	SplinePlotterHandle(SplinePlotter* p) : SplinePlotterRef(p) {}
	~SplinePlotterHandle() { if (*this) destroy(); }
};

namespace Spline {

// stroke vector helpers
// The store is computed by taking the doc-product of these vectors
// with a "cubic parameteric vector", e.g.:
//   U = < u^3, u^2, u, 1 >,
inline vec4f uniformStroke(float u) { return vec4f(0, 0, 0, u); }
inline vec4f taperingStroke(float u, float v) { return vec4f(0, 0, v-u, u); }

inline vec4f eccentricStroke(float t0, float e, float t1) {
	return vec4f(0, -e-e-e-e, e+e+e+e+t1-t0, t0); 
}

inline vec4f quadraticBezierStroke(float t0, float t1, float t2) { 
	return vec4f(0, t0-t1-t1+t2, -t0-t0+t1+t1, t0); 
}

// curve matrix helpers
// These compute hermite curves based on linear multiplication by
// a "cubic parameteric vector", e.g.:
//   U = < u^3, u^2, u, 1 >,

#define XY_ROTATION_MATRIX (mat(0, -1, 0, 0, 1, 0, 0, 0))

inline mat4f derivativeMatrix(mat4f m) {
	// Returns the derivative of the function encoded by the given
	// matrix, which computes the slope of the curve at that point. E.g. 
	// f = Au*3 + Bu^2 + Cu + D
	// f' = 3Au^2 + 2Bu + C
	float mm[16];
	m.store(mm);
	return mat4f ({
		0, 0, 0, 0,
		3*mm[0], 3*mm[1], 3*mm[2], 3*mm[3],
		2*mm[4], 2*mm[5], 2*mm[6], 2*mm[7],
		mm[8], mm[9], mm[10], mm[11]
	});
}

inline mat4f perpendicularMatrix(mat4f m) {
	// Takes the derivaive and then rotates 90-degrees in the 
	// XY-plane to produce planar-normals (useful for "stroke" vectors).
	// XY_ROTATION_MATRIX * derivativeMatrix(m)
	float mm[16];
	m.store(mm);
	return mat4f ({
		0, 0, 0, 0,
		3*mm[1], -3*mm[0], 3*mm[2], 3*mm[3],
		2*mm[5], -2*mm[4], 2*mm[6], 2*mm[7],
		mm[9], -mm[8], mm[10], mm[11]
	});
}

inline mat4f linearMatrix(vec4f p0, vec4f p1) {
	return mat4f(p0, p1, vec4f(0,0,0,0), vec4f(0,0,0,0)) * mat4f({
		0, 0, 0, 0,
		0, 0, 0, 0,
		-1, 1, 0, 0,
		1, 0, 0, 0
	});
}

inline mat4f linearDerivMatrix(vec4f p0, vec4f p1) {
	return mat4f(p0, p1, vec4f(0,0,0,0), vec4f(0,0,0,0)) * mat4f({
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		-1, 1, 0, 0
	});  
}

inline mat4f hermiteMatrix(vec4f p0, vec4f p1, vec4f t0, vec4f t1) {
	return mat4f(p0, p1, t0, t1) * mat4f({
		2, -2, 1, 1, 
		-3, 3, -2, -1, 
		0, 0, 1, 0, 
		1, 0, 0, 0
	});
}

inline mat4f hermiteDerivMatrix(vec4f p0, vec4f p1, vec4f t0, vec4f t1) {
	return mat4f(p0, p1, t0, t1) * mat4f({
		0, 0, 0, 0, 
		6, -6, 3, 3, 
		-6, 6, -4, -2, 
		0, 0, 1, 0
	});
}

inline mat4f bezierMatrix(vec4f p0, vec4f p1, vec4f p2, vec4f p3) {
	return mat4f(p0, p1, p2, p3) * mat4f({
		-1, 3, -3, 1, 
		3, -6, 3, 0, 
		-3, 3, 0, 0, 
		1, 0, 0, 0
	});
}

inline mat4f bezierDerivMatrix(vec4f p0, vec4f p1, vec4f p2, vec4f p3) {
	return mat4f(p0, p1, p2, p3) * mat4f({
		0, 0, 0, 0, 
		-3, 9, -9, 3, 
		6, -12, 6, 0, 
		-3, 3, 0, 0
	});
}

inline mat4f quadraticBezierMatrix(vec4f p0, vec4f p1, vec4f p2) {
	return mat4f(vec4f(0,0,0,0), p0, p1, p2) * mat4f({
		0, 0, 0, 0,
		0, 1, -2, 1,
		0, -2, 2, 0,
		0, 1, 0, 0
	});
}

inline mat4f quadraticBezierDerivMatrix(vec4f p0, vec4f p1, vec4f p2) {
	return mat4f(vec4f(0,0,0,0), p0, p1, p2) * mat4f({
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 2, -4, 2,
		0, -2, 2, 0
	});
}

}
