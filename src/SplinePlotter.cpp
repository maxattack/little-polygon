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

#include "littlepolygon_graphics.h"
#include "littlepolygon_splines.h"

SplinePlotter::SplinePlotter(BasicPlotterRef aPlotter) : plotter(aPlotter), count(-1), curveCapacity(-1) {
}

SplinePlotter::~SplinePlotter() {
}

void SplinePlotter::begin(const Viewport& viewport, GLuint prog) {
	ASSERT(!plotter.isBound());
	ASSERT(!isBound());
	count = 0;
	plotter.begin(viewport, prog);
	
	int w,h;
	SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &w, &h);
	fakeAntiAliasFactor = 0.001 * float(w) / viewport.width();
	glBindTexture(GL_TEXTURE_2D, getFakeAntialiasTexture());
}

inline void computePRV(
	
	// in params
	const mat4f& positionMatrix,
	const mat4f& strokeMatrix,
	const vec4f& strokeVector,
	float u,
	float faaf,

	// out params
	vec2& p,
	vec2& r,
	float& v

) {
	
	// compute pos and stroke vector with fancy-pants SIMD tricks
	vec4f param(u*u*u, u*u, u, 1);
	vec4f centerPos = positionMatrix * param;
	float radius = vectorial::dot(param, strokeVector);
	vec4f stroke = radius * vectorial::normalize(strokeMatrix * param);
	
	// save out-params
	p.x = centerPos.x();
	p.y = centerPos.y();
	r.x = stroke.x();
	r.y = stroke.y();
	v = clamp(0.01 * faaf * radius);

}

void SplinePlotter::reserve(int numVertsRequired) {
	ASSERT(numVertsRequired <= plotter.capacity());
	ASSERT(curveCapacity == -1);
	
	// need to add "degenerate" triangles to separate the two splines
	if (count > 0) {
		numVertsRequired += 2;
	}
	
	// flush if necessary
	if (count > 0 && count + numVertsRequired > plotter.capacity()) {
		flush();
	}
	
}

void SplinePlotter::startCurve(int resolution, vec2 p0, vec2 p1, float z, Color c) {
	ASSERT(isBound());
	reserve(resolution << 1);
	curveCapacity = resolution;
	if (count > 0) {
		// add degenerate triangle
		auto tail = plotter.getVertex(count-1);
		*nextVert() = *tail;
		nextVert()->set(p0, z, vec(0, 1), c);
	}
	nextVert()->set(p0, z, vec(0, 1), c);
	nextVert()->set(p1, z, vec(1, 1), c);
	curveCount = 1;
	
}

void SplinePlotter::plotCurvePoint(vec2 p0, vec2 p1, float z, Color c) {
	ASSERT(isBound());
	ASSERT(curveCapacity > 0 && curveCount < curveCapacity);
	nextVert()->set(p0, z, vec(0, 1), c);
	nextVert()->set(p1, z, vec(1, 1), c);
	curveCount++;
}

void SplinePlotter::finishCurve() {
	ASSERT(isBound());
	ASSERT(curveCount == curveCapacity);
	curveCapacity = -1;
}


void SplinePlotter::plotCubic(const mat4f& posMat, const vec4f& strokeVec, Color c, int resolution) {
	ASSERT(isBound());
	reserve(resolution << 1);
	
	// point computation is done efficiently with matrix math
	auto strokeMat = Spline::perpendicularMatrix(posMat);
	vec2 p,r;
	float v;
	
	// plot vertices
	computePRV(posMat, strokeMat, strokeVec, 0, fakeAntiAliasFactor, p, r, v);
	if (count > 0) {
		// add degenerate triangle
		auto tail = plotter.getVertex(count-1);
		*nextVert() = *tail;
		nextVert()->set(p-r, vec(0,v), c);
	}
	nextVert()->set(p-r, vec(0,v), c);
	nextVert()->set(p+r, vec(1,v), c);
	float du = 1.0 / (resolution-1.0);
	float u = 0;
	for(int i=1; i<resolution; ++i) {
		u += du;
		computePRV(posMat, strokeMat, strokeVec, u, fakeAntiAliasFactor, p, r, v);
		nextVert()->set(p-r, vec(0,v), c);
		nextVert()->set(p+r, vec(1,v), c);
	}
	
}

//inline void setVertex(BasicVertex*v, vec2 p, float z, vec2 uv, Color c) {
//	v->x = p.x;
//	v->y = p.y;
//	v->z = z;
//	v->u = uv.x;
//	v->v = uv.y;
//	v->color = c;
//}
//
void SplinePlotter::plotArc(vec2 p, float z, float r1, float r2, Color c, float a1, float a2, int resolution) {
	ASSERT(isBound());
	reserve(resolution<<1);
	
	float da = a2 - a1;
	float v = clamp(fakeAntiAliasFactor * fabsf(r2-r1));
	float wholeCircle = M_TAU-0.01;

	if (da > wholeCircle || da < -wholeCircle) {
		
		// special case - plotting a closed loop
		// (don't want gaps or overlapped faces)
		vec2 curr = vec(1,0);
		vec2 rotor = unitVector((M_TAU) / (resolution-1));
		
		vec2 p0 = p + r1 * curr;
		vec2 p1 = p + r2 * curr;
		
		if (count > 0) {
			auto tail = plotter.getVertex(count-1);
			*nextVert() = *tail;
			nextVert()->set(p0, z, vec(0,v), c);
		}
		nextVert()->set(p0, vec(0,v), c);
		nextVert()->set(p1, vec(1,v), c);
		for(int i=1; i<resolution-1; ++i) {
			curr = cmul(curr, rotor);
			nextVert()->set(p + r1 * curr, z, vec(0, v), c);
			nextVert()->set(p + r2 * curr, z, vec(1, v), c);
		}
		
		nextVert()->set(p0, z, vec(0,v), c);
		nextVert()->set(p1, z, vec(1,v), c);
		
	} else {
		
		// general case
		vec2 curr = unitVector(a1);
		vec2 rotor = unitVector(da / float(resolution-1));
		
		if (count > 0) {
			auto tail = plotter.getVertex(count-1);
			*nextVert() = *tail;
			nextVert()->set(p + r1 * curr, z, vec(0,v), c);
		}
		nextVert()->set(p + r1 * curr, z, vec(0,v), c);
		nextVert()->set(p + r2 * curr, z, vec(1,v), c);
		for(int i=1; i<resolution; ++i) {
			curr = cmul(curr, rotor);
			nextVert()->set(p + r1 * curr, z, vec(1, v), c);
			nextVert()->set(p + r2 * curr, z, vec(0, v), c);
		}
		
	}
	
}

void SplinePlotter::plotCircle(vec2 p, float z, float r, Color c, int resolution) {
	ASSERT(isBound());
	reserve(resolution<<1);
	vec2 curr = vec(1,0);
	vec2 rotor = unitVector(M_PI / (resolution-1));
	
	if (count > 0) {
		auto tail = plotter.getVertex(count-1);
		*nextVert() = *tail;
		nextVert()->set(p+vec(0,r), z, vec(0,0), c);
	}
	nextVert()->set(p+vec(0,r), z, vec(0,0), c);
	nextVert()->set(p+vec(0,r), z, vec(0,0), c);
	for(int i=1; i<resolution; ++i) {
		curr = cmul(curr, rotor);
		nextVert()->set(p + r * curr, z, vec(1, 1), c);
		nextVert()->set(p + r * curr.conjugate(), z, vec(0, 1), c);
	}
	
	
}

void SplinePlotter::flush() {
	if (count > 0) {
		plotter.commit(count);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, count);
		count = 0;
	}
}

void SplinePlotter::end() {
	ASSERT(isBound());
	flush();
	count = -1;
	plotter.end();
}
