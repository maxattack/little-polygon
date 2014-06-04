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

#include "sprites.h"

/*
 
EXPERIMENTAL: RenderQueues implement a secondary thread for drawing.  Draw
              calls made through this interface will be published over a 
              lockless channel.  For debugging, the renderqueue can also
              run in "immediate mode" which executes the rendering drawing
              directly on the calling thread.

*/

enum DrawCallType {
	kDrawCallBegin,
	kDrawCallImage,
	
	kDrawCallEnd,
};

struct drawcall_t {
	DrawCallType type;
	union {
		struct {
			ImageAsset *image;
			lpMatrix xform;
			int frame;
			Color color;
			Color mod;
		} image;
	}
};

class RenderQueue {
private:
	
	
public:
	RenderQueue();
	~RenderQueue();
	
	// enqueue a draw call to be rendered
	void draw(const drawcall_t& draw);
	
	// finalize the current drawing transation
	void flush();
	
	// has the current transaction completed?
	bool finished() const;
	
private:
	void draw(const drawcall_t& drawcall);
};

