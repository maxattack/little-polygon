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
	Vec2 size() const { return vec((float)w,(float)h); }
	
	void init();
	void bind();
	void release();
	
};

//------------------------------------------------------------------------------
// VIEWPORT

class Viewport {
private:
	Vec2 mSize;
	Vec2 mOffset;

public:
	Viewport() {}
	Viewport(Vec2 aSize, Vec2 aOffset=Vec2(0,0)) :
		mSize(aSize), mOffset(aOffset) {}
	Viewport(float w, float h, float x=0, float y=0) :
		mSize(w,h), mOffset(x,y) {}
	
	Vec2 size() const { return mSize; }
	float width() const { return mSize.x; }
	float height() const { return mSize.y; }
	
	float aspect() const { return mSize.x / mSize.y; }

	Vec2 offset() const { return mOffset; }
	Vec2 extent() const { return mOffset + mSize; }
	float left() const { return mOffset.x; }
	float right() const { return mOffset.x + mSize.x; }
	float top() const { return mOffset.y; }
	float bottom() const { return mOffset.y + mSize.y; }
	
	void setFromWindow();
	void setSize(Vec2 sz) { mSize = sz; }
	void setSize(float w, float h) { mSize.set(w,h); }
	void setSizeWithHeight(float h);
	void setSizeWithWidth(float w);
	void setOffset(Vec2 off) { mOffset = off; }
	void setOffset(float x, float y) { mOffset.set(x,y); }
	
	Vec2 windowToViewport(Vec2 p) const;
	Vec2 viewportToWindow(Vec2 vp) const;

	Vec2 cursor() const;

	void setMVP(GLuint mvp) const;
	
	inline bool contains(Vec2 p, float pad=0.0) const {
		return p.x > mOffset.x - pad &&
		       p.x < mOffset.x + mSize.x + pad &&
		       p.y > mOffset.y - pad &&
		       p.y < mOffset.y + mSize.y + pad;
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
	float x,y,z,u,v;
	Color color;
	
	inline void set(Vec2 p, Vec2 uv, Color c) {
		x = p.x; y = p.y; z = 0; u = uv.x; v = uv.y; color = c;
	}
	
	inline void set(Vec2 p, float az, Vec2 uv, Color c) {
		x = p.x; y = p.y; z = az; u = uv.x; v = uv.y; color = c;
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
		Vec2 position;
		Color color;

		inline void set(Vec2 p, Color c) { 
			position = p; 
			color = c; 
		}
	};

	Array<LineVertex> vertices;

public:
	LinePlotter(int capacity);

	void begin(const Viewport& viewport);
	void plot(Vec2 p0, Vec2 p1, Color c);
	void plotBox(Vec2 p0, Vec2 p2, Color c);
	void plotLittleBox(Vec2 p, float r, Color c);
	void plotArrow(Vec2 p0, Vec2 p1, float r, Color c);
	void end();

private:
	void commitBatch();
};
