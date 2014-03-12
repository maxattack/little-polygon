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
	Viewport() {}
	Viewport(vec2 aSize, vec2 aOffset=vec2(0,0)) :
		mSize(aSize), mOffset(aOffset) {}
	Viewport(float w, float h, float x=0, float y=0) :
		mSize(w,h), mOffset(x,y) {}
	
	vec2 size() const { return mSize; }
	float width() const { return mSize.x; }
	float height() const { return mSize.y; }
	
	float aspect() const { return mSize.x / mSize.y; }

	vec2 offset() const { return mOffset; }
	vec2 extent() const { return mOffset + mSize; }
	float left() const { return mOffset.x; }
	float right() const { return mOffset.x + mSize.x; }
	float top() const { return mOffset.y; }
	float bottom() const { return mOffset.y + mSize.y; }
	
	void setFromWindow();
	void setSize(vec2 sz) { mSize = sz; }
	void setSize(double w, double h) { mSize.set(w,h); }
	void setSizeWithHeight(double h);
	void setSizeWithWidth(double w);
	void setOffset(vec2 off) { mOffset = off; }
	void setOffset(double x, double y) { mOffset.set(x,y); }
	
	vec2 windowToViewport(vec2 p) const;
	vec2 viewportToWindow(vec2 vp) const;

	vec2 mouse() const;

	void setMVP(GLuint mvp) const;
	
	inline bool contains(vec2 p, float pad=0.0) const {
		return p.x > mOffset.x - pad &&
		       p.x < mOffset.x + mSize.x + pad &&
		       p.y > mOffset.y - pad &&
		       p.y < mOffset.y + mSize.y + pad;
	}
};



// Dead-simple shader-compiler.  Easiest to just use C++11 raw string
// literals to store the source, or else you could stash it in asset 
// userdata if it's configurable from content.  Uses a VERTEX conditional-
// compilation macro to differentiate the vertex and fragment shader.
bool compileShader(const GLchar* source, GLuint *outProg, GLuint *outVert, GLuint *outFrag);

struct ShaderHandle {
	GLuint prog, vert, frag;
	
	ShaderHandle(const GLchar *source) {
		CHECK(compileShader(source, &prog, &vert, &frag));
	}
	
	~ShaderHandle() {
		glDeleteProgram(prog);
		glDeleteShader(vert);
		glDeleteShader(frag);
	}
};

typedef Color (*TextureGenerator)(double, double);
GLuint generateTexture(TextureGenerator cb, int w=256, int h=256);
GLuint getFakeAntialiasTexture();

int createRenderToTextureFramebuffer(GLsizei w, GLsizei h, GLuint *outTexture, GLuint *outFramebuffer);

class PostProcessingFX {
private:
	GLuint dfb, fb, rt;    // framebuffer & rendertexture handles
	GLuint prog, vsh, fsh; // shader handles
	GLuint ap, auv;        // attribute locations
	GLuint vbuf;           // vertex buffer handle
	
public:
	PostProcessingFX(const GLchar *source);
	~PostProcessingFX();
	
	// If the fx fail to initialize we'll just fallback on NOOP
	bool valid() const { return fb != 0; }
	
	void release();
	
	void beginScene();
	void endScene();
	void draw();
};

//------------------------------------------------------------------------------
// BASIC PLOTTER
//------------------------------------------------------------------------------

struct BasicPlotter;

BasicPlotter *createBasicPlotter(size_t capacity);

struct BasicVertex {
	float x,y,z,u,v;
	Color color;
	
	inline void set(vec2 p, vec2 uv, Color c) {
		x = p.x;
		y = p.y;
		z = 0;
		u = uv.x;
		v = uv.y;
		color = c;
	}
	
	inline void set(vec3f p, vec2 uv, Color c) {
		p.load(&x);
		u = uv.x;
		v = uv.y;
		color = c;
	}
};

struct BasicPlotterRef {
private:
	BasicPlotter *context;
	
public:
	BasicPlotterRef() {}
	BasicPlotterRef(BasicPlotter *aContext) : context(aContext) {}
	
	operator BasicPlotter*() { return context; }
	operator bool() const { return context; }
	
	void destroy();
	
	const Viewport* view() const;
	BasicVertex *getVertex(int i);
	bool isBound() const;
	int capacity() const;
	
	void begin(const Viewport& view, GLuint program=0);
	
	void commit(int count);

	void end();
	
	

};

class BasicPlotterHandle : public BasicPlotterRef {
public:
	BasicPlotterHandle(size_t capacity) : BasicPlotterRef(createBasicPlotter(capacity)) {}
	~BasicPlotterHandle() { destroy(); }
};

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
	void plotArrow(vec2 p0, vec2 p1, float r, Color c);
	void end();
};

class LinePlotterHandle : public LinePlotterRef {
public:
	LinePlotterHandle(LinePlotter *p) : LinePlotterRef(p) {}
	~LinePlotterHandle() { if (*this) destroy(); }
};
