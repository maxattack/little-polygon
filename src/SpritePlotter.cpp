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

#include "littlepolygon_sprites.h"
#include "littlepolygon_graphics.h"

const GLchar* SPRITE_SHADER = R"GLSL(
varying mediump vec2 uv;
varying mediump vec4 color;

#if VERTEX

uniform highp mat4 mvp;
attribute mediump vec2 aPosition;
attribute mediump vec2 aUv;
attribute mediump vec4 aColor;

void main() {
	gl_Position = mvp * vec4(aPosition, 0, 1);
	color = aColor;
	uv = aUv;
}

#else

uniform lowp sampler2D atlas;

void main() {
	lowp vec4 baseColor = texture2D(atlas, uv);
	gl_FragColor = vec4(mix(baseColor.rgb, color.rgb, color.a), baseColor.a);
}

#endif
)GLSL";

struct SpritePlotter {
	const Viewport *view;
	int capacity;
	int count;

	GLuint prog;
	GLuint vert;
	GLuint frag;
	GLuint uMVP;
	GLuint uAtlas;
	GLuint aPosition;
	GLuint aUV;
	GLuint aColor;	

	// invariant: these two fields need to be adjacent
	GLuint elementBuf;
	GLuint arrayBuf;

	TextureAsset *workingTexture;

	struct Vertex {
		vec2 position;
		vec2 uv;
		Color color;
		
		inline void set(vec2 p, vec2 u, Color c) {
			position = p;
			uv = u;
			color = c;
		}

	};

	Vertex workingBuffer[0];

	Vertex *nextSlice() { return workingBuffer + (4 * count); }
};

// private helper methods

void setTextureAtlas(SpritePlotter* context, TextureAsset* texture);
void commitBatch(SpritePlotter* context);
void plotGlyph(SpritePlotter* context, const GlyphAsset& g, float x, float y, float h, Color c);

static SpritePlotter *allocSpritePlotter(int capacity) {
	return (SpritePlotter*) LITTLE_POLYGON_MALLOC(
		sizeof(SpritePlotter) + 
		sizeof(SpritePlotter::Vertex) * (4 * capacity - 1)
	);
}

static void dealloc(SpritePlotter *context) {
	LITTLE_POLYGON_FREE(context);
}

static void initialize(SpritePlotter* context, int capacity) {
	context->capacity = capacity;
	context->count = -1;
	context->workingTexture = 0;
	CHECK(compileShader(SPRITE_SHADER, &context->prog, &context->vert, &context->frag));
	glUseProgram(context->prog);
	context->uMVP = glGetUniformLocation(context->prog, "mvp");
	context->uAtlas = glGetUniformLocation(context->prog, "atlas");
	context->aPosition = glGetAttribLocation(context->prog, "aPosition");
	context->aUV = glGetAttribLocation(context->prog, "aUv");
	context->aColor = glGetAttribLocation(context->prog, "aColor");

	// setup element array buffer
	uint16_t indices[6 * context->capacity];
	for(int i=0; i<context->capacity; ++i) {
		indices[6*i+0] = 4*i;
		indices[6*i+1] = 4*i+1;
		indices[6*i+2] = 4*i+2;
		indices[6*i+3] = 4*i+2;
		indices[6*i+4] = 4*i+1;
		indices[6*i+5] = 4*i+3;
	}
	glGenBuffers(2, &context->elementBuf); // also generates the arrayBuf
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->elementBuf);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, 
		6 * context->capacity * sizeof(uint16_t), 
		indices, 
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

static void release(SpritePlotter* context) {
	glDeleteBuffers(2, &context->elementBuf); 
	glDeleteProgram(context->prog);
	glDeleteShader(context->vert);
	glDeleteShader(context->frag);
}


SpritePlotterRef createSpritePlotter(int capacity) {
	auto result = allocSpritePlotter(capacity);
	initialize(result, capacity);
	return result;
}

void SpritePlotterRef::destroy() {
	release(context);
	dealloc(context);
}

bool SpritePlotterRef::bound() const { 
	return context->count >= 0; 
}

const Viewport* SpritePlotterRef::view() const {
	return context->view;
}

void SpritePlotterRef::begin(const Viewport& viewport) {
	ASSERT(context->count == -1);
	context->count = 0;

	ASSERT(context->prog);
	glUseProgram(context->prog);
	glEnableVertexAttribArray(context->aPosition);
	glEnableVertexAttribArray(context->aUV);
	glEnableVertexAttribArray(context->aColor);

	context->view = &viewport;
	viewport.setMVP(context->uMVP);

	// bind element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->elementBuf);
	glBindBuffer(GL_ARRAY_BUFFER, context->arrayBuf);

	glVertexAttribPointer(context->aPosition, 2, GL_FLOAT, GL_FALSE, sizeof(SpritePlotter::Vertex), (GLvoid*)0);
	glVertexAttribPointer(context->aUV, 2, GL_FLOAT, GL_FALSE, sizeof(SpritePlotter::Vertex), (GLvoid*)8);
	glVertexAttribPointer(context->aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SpritePlotter::Vertex), (GLvoid*)16);	
}

void SpritePlotterRef::drawImage(ImageAsset *img, vec2 pos, int frame, Color c) {
	ASSERT(context->count >= 0);
	setTextureAtlas(context, img->texture);
	SpritePlotter::Vertex *slice = context->nextSlice();
	FrameAsset *fr = img->frame(frame);

	pos -= fr->pivot();
	
	slice[0].set(pos,               fr->uv0, c);
	slice[1].set(pos+vec(0,fr->h),  fr->uv1, c);
	slice[2].set(pos+vec(fr->w, 0), fr->uv2, c);
	slice[3].set(pos+fr->size(),    fr->uv3, c);

	++context->count;
}

void SpritePlotterRef::drawImage(ImageAsset *img, vec2 pos, vec2 u, int frame, Color c) {
	ASSERT(context->count >= 0);
	setTextureAtlas(context, img->texture);
	SpritePlotter::Vertex *slice = context->nextSlice();
	FrameAsset *fr = img->frame(frame);

	vec2 p0 = -fr->pivot();
	vec2 p1 = p0 + vec(0, fr->h);
	vec2 p2 = p0 + vec(fr->w, 0);
	vec2 p3 = p0 + fr->size();

	slice[0].set(pos+cmul(p0,u), fr->uv0, c);
	slice[1].set(pos+cmul(p1,u), fr->uv1, c);
	slice[2].set(pos+cmul(p2,u), fr->uv2, c);
	slice[3].set(pos+cmul(p3,u), fr->uv3, c);

	++context->count;
}

void SpritePlotterRef::drawImage(ImageAsset *img, const AffineMatrix& xform, int frame, Color c) {
	ASSERT(context->count >= 0);
	setTextureAtlas(context, img->texture);
	SpritePlotter::Vertex *slice = context->nextSlice();
	FrameAsset *fr = img->frame(frame);

	vec2 p0 = -fr->pivot();
	vec2 p1 = p0 + vec(0, fr->h);
	vec2 p2 = p0 + vec(fr->w, 0);
	vec2 p3 = p0 + fr->size();

	slice[0].set(xform.transformPoint(p0), fr->uv0, c);
	slice[1].set(xform.transformPoint(p1), fr->uv1, c);
	slice[2].set(xform.transformPoint(p2), fr->uv2, c);
	slice[3].set(xform.transformPoint(p3), fr->uv3, c);

	++context->count;	
}

void SpritePlotterRef::drawImage(ImageAsset *img, const mat4f& xform, int frame, Color c) {
	ASSERT(context->count >= 0);
	setTextureAtlas(context, img->texture);
	SpritePlotter::Vertex *slice = context->nextSlice();
	FrameAsset *fr = img->frame(frame);

	vec3f p0 = -vec3f(fr->px, fr->py, 0);
	vec3f p1 = p0 + vec3f(0, fr->h, 0);
	vec3f p2 = p0 + vec3f(fr->w, 0, 0);
	vec3f p3 = p0 + vec3f(fr->w, fr->h, 0);

	slice[0].set(transformPoint(xform, p0).xy(), fr->uv0, c);
	slice[1].set(transformPoint(xform, p1).xy(), fr->uv1, c);
	slice[2].set(transformPoint(xform, p2).xy(), fr->uv2, c);
	slice[3].set(transformPoint(xform, p3).xy(), fr->uv3, c);

	++context->count;	
}

void SpritePlotterRef::drawQuad(ImageAsset *img, vec2 p0, vec2 p1, vec2 p2, vec2 p3, int frame, Color c) {
	ASSERT(context->count >= 0);
	setTextureAtlas(context, img->texture);
	SpritePlotter::Vertex *slice = context->nextSlice();
	FrameAsset *fr = img->frame(frame);

	slice[0].set(p0, fr->uv0, c);
	slice[1].set(p1, fr->uv1, c);
	slice[2].set(p2, fr->uv2, c);
	slice[3].set(p3, fr->uv3, c);

	++context->count;	
}

#define UV_LABEL_SLOP (0.0001f)

void plotGlyph(SpritePlotter* context, const GlyphAsset& g, float x, float y, float h, Color c) {
	if (context->count == context->capacity) {
		commitBatch(context);
	}

	SpritePlotter::Vertex *slice = context->nextSlice();
	float k = 1.f / context->workingTexture->w;
	vec2 uv = k * vec(g.x, g.y);
	float du = k * (g.advance-UV_LABEL_SLOP);
	float dv = k * h;

	slice[0].set(vec(x,y), uv, c);	
	slice[1].set(vec(x,y+h), uv+vec(0,dv), c);	
	slice[2].set(vec(x+g.advance,y), uv+vec(du,0), c);	
	slice[3].set(vec(x+g.advance,y+h), uv+vec(du,dv), c);	

	++context->count;
}

void SpritePlotterRef::drawLabel(FontAsset *font, vec2 p, Color c, const char *msg) {
	ASSERT(context->count >= 0);
	setTextureAtlas(context, &(font->texture));

	float px = p.x;
	float py = p.y;
	while(*msg) {
		if (*msg != '\n') {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph(context, g, px, py, font->height, c);
			px += g.advance;
		} else {
			px = p.x;
			py += font->height;
		}
		++msg;
	}

}

void SpritePlotterRef::drawLabelCentered(FontAsset *font, vec2 p, Color c, const char *msg) {
	ASSERT(context->count >= 0);
	setTextureAtlas( context, &(font->texture) );
	float py = p.y;
	while(*msg) {
		int length;
		const char* next = font->measureLine(msg, &length);
		float px = p.x - (length>>1);
		while(msg != next) {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph(context, g, px, py, font->height, c);
			px += g.advance;
			msg++;
		}
		if (*msg == '\n') {
			py += font->height;
			msg++;
		}
	}
}

#define TILE_SLOP (0.001f)

void SpritePlotterRef::drawTilemap(TilemapAsset *map, vec2 position) {
	ASSERT(context->count >= 0);

	// make sure the map is initialized
	map->init();

	vec2 cs = context->view->size() / vec(map->tw, map->th);
	int latticeW = ceilf(cs.x) + 1;
	int latticeH = ceilf(cs.y) + 1;

	vec2 scroll = context->view->offset() - position;
	

	int vox = int(scroll.x/map->tw);
	int voy = int(scroll.y/map->th);
	
	vec2 rem = vec(
		fmod(scroll.x, map->tw),
		fmod(scroll.y, map->th)
	);
	setTextureAtlas(context, &map->tileAtlas);

	float tw = map->tw + TILE_SLOP + TILE_SLOP;
	float th = map->th + TILE_SLOP + TILE_SLOP;
	float uw = (map->tw - TILE_SLOP - TILE_SLOP) / float(map->tileAtlas.w);
	float uh = (map->th - TILE_SLOP - TILE_SLOP) / float(map->tileAtlas.h);
	for(int y=0; y<latticeH; ++y)
	for(int x=0; x<latticeW; ++x) {
		int rawX = x+vox;
		int rawY = y+voy;
		if (rawX >= 0 && rawX < map->mw && rawY >= 0 && rawY < map->mh) {
			uint8_pair_t coord = map->tileAt(rawX, rawY);
			if (coord.x != 0xff) {
				vec2 p = vec(x * map->tw, y * map->th) 
					- vec(TILE_SLOP, TILE_SLOP) 
					- rem + context->view->offset();
				vec2 uv = 
					(vec(map->tw * coord.x, map->th * coord.y) + vec(TILE_SLOP, TILE_SLOP))
					/ vec(map->tileAtlas.w, map->tileAtlas.h);
				SpritePlotter::Vertex *slice = context->nextSlice();

				slice[0].set(p, uv, rgba(0));
				slice[1].set(p+vec(0,th), uv+vec(0,uh), rgba(0));
				slice[2].set(p+vec(tw,0), uv+vec(uw,0), rgba(0));
				slice[3].set(p+vec(tw,th), uv+vec(uw,uh), rgba(0));

				if (++context->count == context->capacity) {
					commitBatch(context);
				}
			}
		}
	}	
}

void SpritePlotterRef::flush() {
	ASSERT(context->count >= 0); 
	if (context->count > 0) { 
		commitBatch(context); 
	}
}

void SpritePlotterRef::end() {
	ASSERT(context->count >= 0);
	flush();
	context->count = -1;
	context->workingTexture = 0;
	glDisableVertexAttribArray(context->aPosition);
	glDisableVertexAttribArray(context->aUV);
	glDisableVertexAttribArray(context->aColor);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void commitBatch(SpritePlotter* context) {
	ASSERT(context->count > 0);
	glBufferData(GL_ARRAY_BUFFER, 4 * context->count * sizeof(SpritePlotter::Vertex), context->workingBuffer, GL_DYNAMIC_DRAW);
	glDrawElements(GL_TRIANGLES, 6 * context->count, GL_UNSIGNED_SHORT, 0);
	context->count = 0;
}

void setTextureAtlas(SpritePlotter* context, TextureAsset *texture) {

	bool atlasChange = texture != context->workingTexture;
	
	// emit a draw call if we're at SPRITE_CAPACITY of if the atlas is changing.
	// (assuming that count will be 0 if workingTexture is null)
	if (context->count == context->capacity || (context->count > 0 && atlasChange)) {
		commitBatch(context);
	}

	// check if we need to bind a new texture
	if (atlasChange) {
		texture->bind();
		context->workingTexture = texture;		
	}
}




