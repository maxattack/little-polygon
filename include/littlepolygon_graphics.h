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

#include "littlepolygon_assets.h"
#include "littlepolygon_math.h"
#include <functional>

//------------------------------------------------------------------------------
// UTIL FUNCTIONS.
//------------------------------------------------------------------------------

// initialize everything the asset system needs (SDL, MIXER, etc).  Uses atexit()
// to register teardown.
SDL_Window *initContext(const char *caption, int w=0, int h=0);

class Viewport {
private:
	vec2 mSize;
	vec2 mOffset;

public:
	Viewport();
	Viewport(vec2 aSize, vec2 aOffset=vec(0,0)) :
		mSize(aSize), mOffset(aOffset) {}

	vec2 size() const { return mSize; }
	float width() const { return mSize.x; }
	float height() const { return mSize.y; }

	vec2 offset() const { return mOffset; }

	vec2 windowToViewport(vec2 p) const;
	vec2 viewportToWindow(vec2 vp) const;

	vec2 mouse() const;

	void setMVP(GLuint mvp) const;
};

// Dead-simple shader-compiler.  Easiest to just use C++11 raw string
// literals to store the source, or else you could stash it in asset 
// userdata if it's configurable from content.  Uses a VERTEX conditional-
// compilation macro to differentiate the vertex and fragment shader.
bool compileShader(const GLchar* source, GLuint *outProg, GLuint *outVert, GLuint *outFrag);

typedef Color (*TextureGenerator)(double, double);
GLuint generateTexture(TextureGenerator cb, int w=256, int h=256);
GLuint getFakeAntialiasTexture();

//------------------------------------------------------------------------------
// DEBUG LINE RENDERING
//------------------------------------------------------------------------------

struct LinePlotter;
class LinePlotterRef;

LinePlotterRef createLinePlotter();

class LinePlotterRef {
private:
	LinePlotter *context;

public:
	LinePlotterRef() {}
	LinePlotterRef(LinePlotter *aContext) : context(aContext) {}

	operator LinePlotter*() { return context; }
	operator bool() const { return context; }

	void destroy();

	void begin(const Viewport& viewport);
	void plot(vec2 p0, vec2 p1, Color c);
	void plotLittleBox(vec2 p, float r, Color c);
	void end();
};

//------------------------------------------------------------------------------
// CIRCLE RENDERING
//------------------------------------------------------------------------------

struct CirclePlotter;
class CirclePlotterRef;

CirclePlotterRef createCirclePlotter(size_t resolution=128);

class CirclePlotterRef {
private:
	CirclePlotter *context;

public:
	CirclePlotterRef() {}
	CirclePlotterRef(CirclePlotter *aContext) : context(aContext) {}

	operator CirclePlotter*() { return context; }
	operator bool() const { return context; }

	void destroy();

	void begin(const Viewport& viewport);
	void plotFilled(vec2 p, float r, Color c, float a1=0, float a2=M_TAU);
	void plotArc(vec2 p, float r1, float r2, Color c, float a1=0, float a2=M_TAU);
	void end();
};

