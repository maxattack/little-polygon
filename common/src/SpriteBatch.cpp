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

#include "littlepolygon_utils.h"

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

SpriteBatch::SpriteBatch() : count(-1), workingTexture(0) {
	CHECK(compileShader(SPRITE_SHADER, &prog, &vert, &frag));
	glUseProgram(prog);
	uMVP = glGetUniformLocation(prog, "mvp");
	uAtlas = glGetUniformLocation(prog, "atlas");
	aPosition = glGetAttribLocation(prog, "aPosition");
	aUV = glGetAttribLocation(prog, "aUv");
	aColor = glGetAttribLocation(prog, "aColor");

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
	glGenBuffers(2, &elementBuf); // also generates the arrayBuf
	                              // because it's adjacent
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuf);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, 
		6 * SPRITE_CAPACITY * sizeof(uint16_t), 
		indices, 
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

SpriteBatch::~SpriteBatch() {
	glDeleteBuffers(2, &elementBuf); 
	glDeleteProgram(prog);
	glDeleteShader(vert);
	glDeleteShader(frag);	
}

void SpriteBatch::begin(vec2 aCanvasSize, vec2 aCanvasOffset) {
	ASSERT(count == -1);
	count = 0;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	ASSERT(prog);
	glUseProgram(prog);
	glEnableVertexAttribArray(aPosition);
	glEnableVertexAttribArray(aUV);
	glEnableVertexAttribArray(aColor);

	canvasSize = aCanvasSize;
	canvasScroll = aCanvasOffset;
	setCanvas(uMVP, canvasSize, canvasScroll);

	// bind element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuf);
	glBindBuffer(GL_ARRAY_BUFFER, arrayBuf);

	glVertexAttribPointer(aPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)8);
	glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLvoid*)16);	
}

void SpriteBatch::drawImage(ImageAsset *img, vec2 pos, int frame, Color c) {
	ASSERT(count >= 0);
	setTextureAtlas(img->texture);
	Vertex *slice = &workingBuffer[4 * count];
	FrameAsset *fr = img->frame(frame);

	pos -= vec(fr->px, fr->py);
	
	slice[0].set(pos,                   vec(fr->u0, fr->v0), c);
	slice[1].set(pos+vec(0,fr->h),      vec(fr->u1, fr->v1), c);
	slice[2].set(pos+vec(fr->w, 0),     vec(fr->u2, fr->v2), c);
	slice[3].set(pos+vec(fr->w, fr->h), vec(fr->u3, fr->v3), c);

	++count;
}

void SpriteBatch::drawImageTransformed(ImageAsset *img, vec2 pos, vec2 u, int frame, Color c) {
	ASSERT(count >= 0);
	setTextureAtlas(img->texture);
	Vertex *slice = &workingBuffer[4 * count];
	FrameAsset *fr = img->frame(frame);

	vec2 p0 = -vec(fr->px, fr->py);
	vec2 p1 = p0 + vec(0, fr->h);
	vec2 p2 = p0 + vec(fr->w, 0);
	vec2 p3 = p0 + vec(fr->w, fr->h);

	slice[0].set(pos+cmul(p0,u), vec(fr->u0, fr->v0), c);
	slice[1].set(pos+cmul(p1,u), vec(fr->u1, fr->v1), c);
	slice[2].set(pos+cmul(p2,u), vec(fr->u2, fr->v2), c);
	slice[3].set(pos+cmul(p3,u), vec(fr->u3, fr->v3), c);

	++count;
}

void SpriteBatch::drawImageRotated(ImageAsset *img, vec2 pos, float radians, int f, Color c) {
	drawImageTransformed(img, pos, polar(1, radians), f, c);
}

void SpriteBatch::drawImageScaled(ImageAsset *img, vec2 pos, vec2 k, int frame, Color c) {
	ASSERT(count >= 0);
	setTextureAtlas(img->texture);
	Vertex *slice = &workingBuffer[4 * count];
	FrameAsset *fr = img->frame(frame);

	vec2 p0 = -vec(fr->px, fr->py);
	vec2 p1 = p0 + vec(0, fr->h);
	vec2 p2 = p0 + vec(fr->w, 0);
	vec2 p3 = p0 + vec(fr->w, fr->h);

	slice[0].set(pos+k*p0, vec(fr->u0, fr->v0), c);
	slice[1].set(pos+k*p1, vec(fr->u1, fr->v1), c);
	slice[2].set(pos+k*p2, vec(fr->u2, fr->v2), c);
	slice[3].set(pos+k*p3, vec(fr->u3, fr->v3), c);

	++count;	
}


#define UV_LABEL_SLOP (0.0001f)

void SpriteBatch::plotGlyph(const GlyphAsset& g, float x, float y, float h, Color c) {

	if (count == SPRITE_CAPACITY) {
		commitBatch();
	}

	Vertex *slice = &workingBuffer[4 * count];
	float k = 1.f / workingTexture->w;
	vec2 uv = k * vec(g.x, g.y);
	float du = k * (g.advance-UV_LABEL_SLOP);
	float dv = k * h;

	slice[0].set(vec(x,y), uv, c);	
	slice[1].set(vec(x,y+h), uv+vec(0,dv), c);	
	slice[2].set(vec(x+g.advance,y), uv+vec(du,0), c);	
	slice[3].set(vec(x+g.advance,y+h), uv+vec(du,dv), c);	

	++count;
}

void SpriteBatch::drawLabel(FontAsset *font, vec2 p, Color c, const char *msg) {
	ASSERT(count >= 0);
	setTextureAtlas( &(font->texture) );

	float px = p.x;
	float py = p.y;
	while(*msg) {
		if (*msg != '\n') {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph(g, px, py, font->height, c);
			px += g.advance;
		} else {
			px = p.x;
			py += font->height;
		}
		++msg;
	}

}

void SpriteBatch::drawLabelCentered(FontAsset *font, vec2 p, Color c, const char *msg) {
	ASSERT(count >= 0);
	setTextureAtlas( &(font->texture) );
	float py = p.y;
	while(*msg) {
		int length;
		const char* next = font->measureLine(msg, &length);
		float px = p.x - (length>>1);
		while(msg != next) {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph(g, px, py, font->height, c);
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

void SpriteBatch::drawTilemap(TilemapAsset *map, vec2 position) {
	ASSERT(count >= 0);

	// make sure the map is initialized
	map->init();

	// flush the draw queue first so we can turn off blending
	flush();
	glDisable(GL_BLEND);

	vec2 cs = canvasSize / vec(map->tw, map->th);	
	int latticeW = ceilf(cs.x) + 1;
	int latticeH = ceilf(cs.y) + 1;

	vec2 scroll = canvasScroll -position;
	

	int vox = int(scroll.x/map->tw);
	int voy = int(scroll.y/map->th);
	
	vec2 rem = vec(
		fmod(scroll.x, map->tw),
		fmod(scroll.y, map->th)
	);
	setTextureAtlas(&map->tileAtlas);

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
					- rem + canvasScroll;
				vec2 uv = 
					(vec(map->tw * coord.x, map->th * coord.y) + vec(TILE_SLOP, TILE_SLOP))
					/ vec(map->tileAtlas.w, map->tileAtlas.h);
				Vertex *slice = &workingBuffer[4 * count];

				slice[0].set(p, uv, rgba(0));
				slice[1].set(p+vec(0,th), uv+vec(0,uh), rgba(0));
				slice[2].set(p+vec(tw,0), uv+vec(uw,0), rgba(0));
				slice[3].set(p+vec(tw,th), uv+vec(uw,uh), rgba(0));

				if (++count == SPRITE_CAPACITY) {
					commitBatch();
				}
			}
		}
	}	

	// flush the queue again so we can re-enable blending
	flush();
	glEnable(GL_BLEND);
}

void SpriteBatch::flush() {
	ASSERT(count >= 0); 
	if (count > 0) { 
		commitBatch(); 
	}
}

void SpriteBatch::end() {
	ASSERT(count >= 0);
	flush();
	count = -1;
	workingTexture = 0;
	glDisableVertexAttribArray(aPosition);
	glDisableVertexAttribArray(aUV);
	glDisableVertexAttribArray(aColor);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SpriteBatch::commitBatch() {
	ASSERT(count > 0);
	glBufferData(GL_ARRAY_BUFFER, 4 * count * sizeof(Vertex), workingBuffer, GL_DYNAMIC_DRAW);
	glDrawElements(GL_TRIANGLES, 6 * count, GL_UNSIGNED_SHORT, 0);
	count = 0;
}

void SpriteBatch::setTextureAtlas(TextureAsset *texture) {

	bool atlasChange = texture != workingTexture;
	
	// emit a draw call if we're at SPRITE_CAPACITY of if the atlas is changing.
	// (assuming that count will be 0 if workingTexture is null)
	if (count == SPRITE_CAPACITY || (count > 0 && atlasChange)) {
		commitBatch();
	}

	// check if we need to bind a new texture
	if (atlasChange) {
		texture->bind();
		workingTexture = texture;		
	}
}



