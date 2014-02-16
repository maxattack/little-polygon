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

SplinePlotter::SplinePlotter(BasicPlotterRef aPlotter) : plotter(aPlotter), count(-1) {
}

SplinePlotter::~SplinePlotter() {
}

void SplinePlotter::begin(const Viewport& viewport) {
	ASSERT(!plotter.isBound());
	ASSERT(!isBound());
	count = 0;
	plotter.begin(viewport);
	
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
	
	// need to add "degenerate" triangles to separate the two splines
	if (count > 0) {
		numVertsRequired += 2;
	}
	
	// flush if necessary
	if (count > 0 && count + numVertsRequired > plotter.capacity()) {
		flush();
	}
	
}

void SplinePlotter::plotCubic(mat4f posMat, vec4f strokeVec, Color c, int resolution) {
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

void SplinePlotter::plotArc(vec2 p, float r1, float r2, Color c, float a1, float a2, int resolution) {
	ASSERT(isBound());
	reserve(resolution<<1);
	
	// plot eet
	vec2 curr = unitVector(a1);
	float da = (a2 - a1) / float(resolution-1);
	vec2 rotor = unitVector(da);
	float v = clamp(fakeAntiAliasFactor * fabsf(r2-r1));
	
	if (count > 0) {
		auto tail = plotter.getVertex(count-1);
		*nextVert() = *tail;
		nextVert()->set(p + r1 * curr, vec(0,v), c);
	}
	nextVert()->set(p + r1 * curr, vec(0,v), c);
	nextVert()->set(p + r2 * curr, vec(1,v), c);
	for(int i=1; i<resolution; ++i) {
		curr = cmul(curr, rotor);
		nextVert()->set(p + r1 * curr, vec(0, v), c);
		nextVert()->set(p + r2 * curr, vec(1, v), c);
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
	glBindTexture(GL_TEXTURE_2D, 0);
	plotter.end();
}
