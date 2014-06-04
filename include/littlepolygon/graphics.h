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
#include "math.h"
#include "collections.h"

//------------------------------------------------------------------------------
// ASSETS

#define TEXTURE_FLAG_FILTER  0x1
#define TEXTURE_FLAG_REPEAT  0x2
#define TEXTURE_FLAG_LUM     0x4
#define TEXTURE_FLAG_RGB     0x8

struct TextureAsset
{
	
	void*    compressedData;  // zlib compressed texture data
	int32_t  w, h;            // size of the texture (guarenteed to be POT)
	uint32_t compressedSize,  // size of the compressed buffer, in bytes
	         handle,          // handle to the initialized texture resource
	         flags;           // extra information (wrapping, format, etc)
	
	bool initialized() const { return handle != 0; }
	int format() const { return GL_RGBA; }
	lpVec size() const { return vec((lpFloat)w,(lpFloat)h); }
	
	void init();
	void bind();
	void release();
	
};

//------------------------------------------------------------------------------
// VIEWPORT

class Viewport {
private:
	lpVec mHalfSize;
	lpVec mCenter;

public:
	Viewport() {}
	Viewport(lpVec aSize, lpVec aCenter=lpVec(0,0)) :
		mHalfSize(0.5*aSize), mCenter(aCenter) {}
	Viewport(lpFloat w, lpFloat h, lpFloat x=0, lpFloat y=0) :
		mHalfSize(0.5*w,0.5*h), mCenter(x,y) {}
	
	Viewport scaled(lpFloat k) { return Viewport(2.0f*k*mHalfSize, k*mCenter); }
	
	lpVec size() const { return 2.0 * mHalfSize; }
	lpFloat width() const { return 2.0 * mHalfSize.x; }
	lpFloat height() const { return 2.0 * mHalfSize.y; }
	
	lpFloat aspect() const { return mHalfSize.x / mHalfSize.y; }

	lpVec center() const { return mCenter; }
	lpVec offset() const { return mCenter - mHalfSize; }
	lpVec extent() const { return mCenter + mHalfSize; }
	lpFloat left() const { return mCenter.x - mHalfSize.x; }
	lpFloat right() const { return mCenter.x + mHalfSize.x; }
	lpFloat top() const { return mCenter.y - mHalfSize.y; }
	lpFloat bottom() const { return mCenter.y + mHalfSize.y; }
	
	void setFromWindow();
	void setSize(lpVec sz) { mHalfSize = 0.5 * sz; }
	void setSize(lpFloat w, lpFloat h) { mHalfSize.set(0.5*w,0.5*h); }
	void setSizeWithHeight(lpFloat h);
	void setSizeWithWidth(lpFloat w);
	void setCenter(lpVec center) { mCenter = center; }
	void setCenter(float x, float y) { mCenter.set(x,y); }
	void setOffset(lpVec off) { mCenter = off + mHalfSize; }
	void setOffset(lpFloat x, lpFloat y) { mCenter.set(x-mHalfSize.x, y-mHalfSize.y); }
	void move(lpVec delta) { mCenter += delta; }
	void move(lpFloat x, lpFloat y) { mCenter += vec(x,y); }
	
	lpVec windowToViewport(lpVec p) const;
	lpVec viewportToWindow(lpVec vp) const;

	lpVec cursor() const;

	void setMVP(GLuint mvp) const;
	
	inline bool contains(lpVec p, lpFloat pad=0.0) const {
		return lpAbs(p.x - mCenter.x) < mHalfSize.x + pad &&
		       lpAbs(p.y - mCenter.y) < mHalfSize.y + pad;
	}
};

//------------------------------------------------------------------------------
// SHADER

#define GLSL(src) "#version 150 core\n" #src

struct Shader {
	GLuint prog, vert, frag;
	
	Shader(const GLchar *vsrc, const GLchar *fsrc);
	~Shader();

	bool isValid() const { return prog != 0; }
	void use() { ASSERT(prog); glUseProgram(prog); }
	GLuint uniformLocation(const char *name) { ASSERT(prog); return glGetUniformLocation(prog, name); }
	GLuint attribLocation(const char *name) { ASSERT(prog); return glGetAttribLocation(prog, name); }

};

//------------------------------------------------------------------------------
// TEXTURES

typedef Color (*TextureGenerator)(double, double);
GLuint generateTexture(TextureGenerator cb, int w=256, int h=256);

//------------------------------------------------------------------------------
// DYNAMIC PLOTTER

struct Vertex {
	GLfloat x,y,u,v;
	Color c1,c2;
	
	inline void set(lpVec p, lpVec uv, Color c, Color t=rgba(0xffffffff))
	{
		x = (GLfloat) p.x;
		y = (GLfloat) p.y;
		u = (GLfloat) uv.x;
		v = (GLfloat) uv.y;
		c1 = c;
		c2 = t;
	}
	
};

class Plotter {
private:
	int capacity;
	int currentArray;
	GLuint vbo[3];
	Array<Vertex> vertices;
	
public:
	Plotter(int capacity);
	~Plotter();
	
	int getCapacity() const { return capacity; }
	GLuint getVBO(int i) { ASSERT(i >= 0 && i < 3); return vbo[i]; }
	Vertex *getVertex(int i) { ASSERT(i >= 0 && i < capacity); return &vertices[i]; }
	
	int getCurrentArray() { return currentArray; }
	void swapBuffer();
	
	void bufferData(int count);
};

//------------------------------------------------------------------------------
// POST-PROCESSING

//int createRenderToTextureFramebuffer(GLsizei w, GLsizei h, GLuint *outTexture, GLuint *outFramebuffer);
//
//class PostProcessingFX {
//private:
//	GLuint dfb, fb, rt;    // framebuffer & rendertexture handles
//	Shader shader;
//	GLuint ap, auv;        // attribute locations
//	GLuint vbuf;           // vertex buffer handle
//	
//public:
//	PostProcessingFX(const GLchar *source);
//	~PostProcessingFX();
//	
//	// If the fx fail to initialize we'll just fallback on NOOP
//	bool valid() const { return fb != 0; }
//	
//	void release();
//	
//	void beginScene();
//	void endScene();
//	void draw();
//};
//

//------------------------------------------------------------------------------
// DEBUG LINE RENDERING

class LinePlotter {
private:
	int count, capacity;
	Shader shader;
	GLuint vao, vbo, uMVP, aPosition, aColor;

	struct LineVertex {
		GLfloat x, y;
		Color color;

		inline void set(lpVec p, Color c) { 
			x = (GLfloat) p.x;
			y = (GLfloat) p.y;
			color = c; 
		}
	};

	Array<LineVertex> vertices;

public:
	LinePlotter(int capacity);

	void begin(const Viewport& viewport);
	void plot(lpVec p0, lpVec p1, Color c);
	void plotBox(lpVec p0, lpVec p2, Color c);
	void plotLittleBox(lpVec p, lpFloat r, Color c);
	void plotArrow(lpVec p0, lpVec p1, lpFloat r, Color c);
	void end();

private:
	void commitBatch();
};
