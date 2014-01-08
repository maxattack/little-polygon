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

const GLchar* SPRITE_SHADER = R"GLSL(
varying mediump vec2 uv;
varying mediump vec4 color;

#if VERTEX

uniform mediump mat4 mvp;
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
	vec4 baseColor = texture2D(atlas, uv);
	gl_FragColor = vec4(mix(baseColor.rgb, color.rgb, color.a), baseColor.a);
}

#endif
)GLSL";

#define SPRITE_CAPACITY 256

struct SpriteBatch {
	vec2 canvasSize;
	vec2 canvasScroll;
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

	Vertex workingBuffer[4 * SPRITE_CAPACITY]; // four corners
	TextureAsset *workingTexture;
};

// private helper methods

void setTextureAtlas(SpriteBatch* context, TextureAsset* texture);
void commitBatch(SpriteBatch* context);
void plotGlyph(SpriteBatch* context, const GlyphAsset& g, float x, float y, float h, Color c);

SpriteBatch *newSpriteBatch() {
	auto result = allocSpriteBatch();
	initialize(result);
	return result;
}

void destroy(SpriteBatch *context) {
	release(context);
	dealloc(context);
}


SpriteBatch *allocSpriteBatch() {
	return (SpriteBatch*) LITTLE_POLYGON_MALLOC(sizeof(SpriteBatch));
}

void dealloc(SpriteBatch *context) {
	LITTLE_POLYGON_FREE(context);
}

void initialize(SpriteBatch* context) {
	context->count = -1;
	context->workingTexture = 0;
	CHECK(compileShader(SPRITE_SHADER, &context->prog, &context->vert, &context->frag));
	glUseProgram(context->prog);
	context->uMVP = glGetUniformLocation(context->prog, "mvp");
	context->uAtlas = glGetUniformLocation(context->prog, "atlas");
	context->aPosition = glGetAttribLocation(context->prog, "aPosition");
	context->aUV = glGetAttribLocation(context->prog, "aUv");
	context->aColor = glGetAttribLocation(context->prog, "aColor");

	// setup element array buffer (should this be static?)
	uint16_t indices[6 * SPRITE_CAPACITY];
	for(int i=0; i<SPRITE_CAPACITY; ++i) {
		indices[6*i+0] = 4*i;
		indices[6*i+1] = 4*i+1;
		indices[6*i+2] = 4*i+2;
		indices[6*i+3] = 4*i+2;
		indices[6*i+4] = 4*i+1;
		indices[6*i+5] = 4*i+3;
	}
	glGenBuffers(2, &context->elementBuf); // also generates the arrayBuf
	                                      // because it's adjacent
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->elementBuf);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, 
		6 * SPRITE_CAPACITY * sizeof(uint16_t), 
		indices, 
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void release(SpriteBatch* context) {
	glDeleteBuffers(2, &context->elementBuf); 
	glDeleteProgram(context->prog);
	glDeleteShader(context->vert);
	glDeleteShader(context->frag);
}

void begin(SpriteBatch* context, vec2 aCanvasSize, vec2 aCanvasOffset) {
	ASSERT(context->count == -1);
	context->count = 0;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	ASSERT(context->prog);
	glUseProgram(context->prog);
	glEnableVertexAttribArray(context->aPosition);
	glEnableVertexAttribArray(context->aUV);
	glEnableVertexAttribArray(context->aColor);

	context->canvasSize = aCanvasSize;
	context->canvasScroll = aCanvasOffset;
	setCanvas(context->uMVP, context->canvasSize, context->canvasScroll);

	// bind element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->elementBuf);
	glBindBuffer(GL_ARRAY_BUFFER, context->arrayBuf);

	glVertexAttribPointer(context->aPosition, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteBatch::Vertex), (GLvoid*)0);
	glVertexAttribPointer(context->aUV, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteBatch::Vertex), (GLvoid*)8);
	glVertexAttribPointer(context->aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SpriteBatch::Vertex), (GLvoid*)16);	
}

void drawImage(SpriteBatch* context, ImageAsset *img, vec2 pos, int frame, Color c) {
	ASSERT(context->count >= 0);
	setTextureAtlas(context, img->texture);
	SpriteBatch::Vertex *slice = &context->workingBuffer[4 * context->count];
	FrameAsset *fr = img->frame(frame);

	pos -= vec(fr->px, fr->py);
	
	slice[0].set(pos,                   vec(fr->u0, fr->v0), c);
	slice[1].set(pos+vec(0,fr->h),      vec(fr->u1, fr->v1), c);
	slice[2].set(pos+vec(fr->w, 0),     vec(fr->u2, fr->v2), c);
	slice[3].set(pos+vec(fr->w, fr->h), vec(fr->u3, fr->v3), c);

	++context->count;
}

void drawImageTransformed(SpriteBatch* context, ImageAsset *img, vec2 pos, vec2 u, int frame, Color c) {
	ASSERT(context->count >= 0);
	setTextureAtlas(context, img->texture);
	SpriteBatch::Vertex *slice = &context->workingBuffer[4 * context->count];
	FrameAsset *fr = img->frame(frame);

	vec2 p0 = -vec(fr->px, fr->py);
	vec2 p1 = p0 + vec(0, fr->h);
	vec2 p2 = p0 + vec(fr->w, 0);
	vec2 p3 = p0 + vec(fr->w, fr->h);

	slice[0].set(pos+cmul(p0,u), vec(fr->u0, fr->v0), c);
	slice[1].set(pos+cmul(p1,u), vec(fr->u1, fr->v1), c);
	slice[2].set(pos+cmul(p2,u), vec(fr->u2, fr->v2), c);
	slice[3].set(pos+cmul(p3,u), vec(fr->u3, fr->v3), c);

	++context->count;
}

void drawImageRotated(SpriteBatch* context, ImageAsset *img, vec2 pos, float radians, int f, Color c) {
	drawImageTransformed(context, img, pos, polar(1, radians), f, c);
}

void drawImageScaled(SpriteBatch* context, ImageAsset *img, vec2 pos, vec2 k, int frame, Color c) {
	ASSERT(context->count >= 0);
	setTextureAtlas(context, img->texture);
	SpriteBatch::Vertex *slice = &context->workingBuffer[4 * context->count];
	FrameAsset *fr = img->frame(frame);

	vec2 p0 = -vec(fr->px, fr->py);
	vec2 p1 = p0 + vec(0, fr->h);
	vec2 p2 = p0 + vec(fr->w, 0);
	vec2 p3 = p0 + vec(fr->w, fr->h);

	slice[0].set(pos+k*p0, vec(fr->u0, fr->v0), c);
	slice[1].set(pos+k*p1, vec(fr->u1, fr->v1), c);
	slice[2].set(pos+k*p2, vec(fr->u2, fr->v2), c);
	slice[3].set(pos+k*p3, vec(fr->u3, fr->v3), c);

	++context->count;	
}


#define UV_LABEL_SLOP (0.0001f)

void plotGlyph(SpriteBatch* context, const GlyphAsset& g, float x, float y, float h, Color c) {
	if (context->count == SPRITE_CAPACITY) {
		commitBatch(context);
	}

	SpriteBatch::Vertex *slice = &context->workingBuffer[4 * context->count];
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

void drawLabel(SpriteBatch* context, FontAsset *font, vec2 p, Color c, const char *msg) {
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

void drawLabelCentered(SpriteBatch* context, FontAsset *font, vec2 p, Color c, const char *msg) {
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

void drawTilemap(SpriteBatch* context, TilemapAsset *map, vec2 position) {
	ASSERT(context->count >= 0);

	// make sure the map is initialized
	initialize(map);

	// flush the draw queue first so we can turn off blending
	flush(context);
	glDisable(GL_BLEND);

	vec2 cs = context->canvasSize / vec(map->tw, map->th);	
	int latticeW = ceilf(cs.x) + 1;
	int latticeH = ceilf(cs.y) + 1;

	vec2 scroll = context->canvasScroll - position;
	

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
					- rem + context->canvasScroll;
				vec2 uv = 
					(vec(map->tw * coord.x, map->th * coord.y) + vec(TILE_SLOP, TILE_SLOP))
					/ vec(map->tileAtlas.w, map->tileAtlas.h);
				SpriteBatch::Vertex *slice = &context->workingBuffer[4 * context->count];

				slice[0].set(p, uv, rgba(0));
				slice[1].set(p+vec(0,th), uv+vec(0,uh), rgba(0));
				slice[2].set(p+vec(tw,0), uv+vec(uw,0), rgba(0));
				slice[3].set(p+vec(tw,th), uv+vec(uw,uh), rgba(0));

				if (++context->count == SPRITE_CAPACITY) {
					commitBatch(context);
				}
			}
		}
	}	

	// flush the queue again so we can re-enable blending
	flush(context);
	glEnable(GL_BLEND);
}

void flush(SpriteBatch* context) {
	ASSERT(context->count >= 0); 
	if (context->count > 0) { 
		commitBatch(context); 
	}
}

void end(SpriteBatch* context) {
	ASSERT(context->count >= 0);
	flush(context);
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

void commitBatch(SpriteBatch* context) {
	ASSERT(context->count > 0);
	glBufferData(GL_ARRAY_BUFFER, 4 * context->count * sizeof(SpriteBatch::Vertex), context->workingBuffer, GL_DYNAMIC_DRAW);
	glDrawElements(GL_TRIANGLES, 6 * context->count, GL_UNSIGNED_SHORT, 0);
	context->count = 0;
}

void setTextureAtlas(SpriteBatch* context, TextureAsset *texture) {

	bool atlasChange = texture != context->workingTexture;
	
	// emit a draw call if we're at SPRITE_CAPACITY of if the atlas is changing.
	// (assuming that count will be 0 if workingTexture is null)
	if (context->count == SPRITE_CAPACITY || (context->count > 0 && atlasChange)) {
		commitBatch(context);
	}

	// check if we need to bind a new texture
	if (atlasChange) {
		bind(texture);
		context->workingTexture = texture;		
	}
}




