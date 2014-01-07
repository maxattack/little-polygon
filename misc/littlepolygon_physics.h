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

// include box2d before utils so that we get implicit castying
// between LP and B2 math types.
#include <littlepolygon_utils.h>

#ifndef PIXELS_PER_METER
#define PIXELS_PER_METER 1
#define METERS_PER_PIXEL 1
#else
#define METERS_PER_PIXEL (1.0f/PIXELS_PER_METER)
#endif

#ifdef BOX2D_H

class WireframeDraw : public b2Draw, public LinePlotter {
public:

	WireframeDraw() {
		SetFlags( e_shapeBit | e_jointBit | e_pairBit  | e_centerOfMassBit );
	}

	void doPlot(b2Vec2 p0, b2Vec2 p1, const b2Color& color) {
		plot(PIXELS_PER_METER * p0, PIXELS_PER_METER * p1, color);
	}

	void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
		for(int i=0; i<vertexCount-1; ++i) {
			doPlot(vertices[i], vertices[i+1], color);
		}
		doPlot(vertices[vertexCount-1], vertices[0], color);
	}

	void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
		DrawPolygon(vertices, vertexCount, color);
	}

	void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) {
		const int resolution = 32;
		vec2 curr = vec(radius, 0);
		vec2 rotator = polar(1, 1.0f/resolution);
		for(int i=0; i<resolution; ++i) {
			auto next = cmul(curr, rotator);
			doPlot(center + curr, center + next, color);
		}
	}

	void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) {
		DrawCircle(center, radius, color);
		doPlot(center-axis, center+axis, color);		
	}
	void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {
		doPlot(p1, p2, color);		
	}

	void DrawTransform(const b2Transform& xf) {
		doPlot(xf.p, xf.p + 0.25f * vec(xf.q.c, xf.q.s), rgb(0xff0000));
		doPlot(xf.p, xf.p + 0.25f * vec(-xf.q.s, xf.q.c), rgb(0x00ff00));
	}
};

#endif
