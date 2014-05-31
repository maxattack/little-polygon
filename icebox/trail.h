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

#include "graphics.h"

// TODO: Texture-mapping and/or ramping along the trail?

struct TrailBatch;

TrailBatch* createTrailBatch(size_t capacity=1024);

class TrailBatchRef {
private:
	TrailBatch *context;

public:
	TrailBatchRef() {}
	TrailBatchRef(TrailBatch *p) : context(p) {}
	
	operator bool() const { return context != 0; }
	operator TrailBatch*() { return context; }
	operator TrailBatch*() const { return context; }
	
	Color color() const;
	float stroke() const;
	float time() const;
	float fadeDuration() const;
	
	void setColor(Color c);
	void setStroke(float w);
	void setFadeDuration(float seconds);
	
	void clear();
	
	void append(vec2 position, float z, float stroke=1, float minDist=0.05);
	
	void tick(float deltaSeconds);
	
	void draw(const Viewport &view);
	
	void destroy();
};

class TrailBatchHandle : public TrailBatchRef {
public:
	TrailBatchHandle(TrailBatch *p) : TrailBatchRef(p) {}
	~TrailBatchHandle() { destroy(); }
};