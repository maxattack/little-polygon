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

#include "littlepolygon/sprites.h"

const GLchar SPRITE_VERT[] = GLSL(

uniform mat4 mvp;
in vec3 aPosition;
in vec2 aUv;
in vec4 aColor;
out vec2 uv;
out vec4 color;

void main() {
	gl_Position = mvp * vec4(aPosition, 1.0);
	color = aColor;
	uv = aUv;
}

);

const GLchar SPRITE_FRAG[] = GLSL(

uniform sampler2D atlas;
in vec2 uv;
in vec4 color;
out vec4 outColor;

void main() {
	vec4 baseColor = texture(atlas, uv);
	outColor = vec4(mix(baseColor.rgb, color.rgb, color.a), baseColor.a);
	//outColor = vec4(mix(baseColor.rgb, baseColor.a * color.rgb, color.a), baseColor.a);
}

);

SpritePlotter::SpritePlotter(Plotter *aPlotter) :
plotter(aPlotter),
count(-1),
shader(SPRITE_VERT, SPRITE_FRAG),
workingTexture(0)
{
	
	// initialize shader
	shader.use();
	uMVP = shader.uniformLocation("mvp");
	uAtlas = shader.uniformLocation("atlas");
	aPosition = shader.attribLocation("aPosition");
	aUV = shader.attribLocation("aUv");
	aColor = shader.attribLocation("aColor");
	
	// setup element array buffer
	uint16_t *indices = (uint16_t*)SDL_malloc(6 * capacity() * sizeof(uint16_t));
	for(uint16_t i=0; i<capacity(); ++i) {
		indices[6*i+0] = 4*i;
		indices[6*i+1] = 4*i+1;
		indices[6*i+2] = 4*i+2;
		indices[6*i+3] = 4*i+2;
		indices[6*i+4] = 4*i+1;
		indices[6*i+5] = 4*i+3;
	}
	glGenBuffers(1, &elementBuf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuf);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, 
		6 * capacity() * sizeof(uint16_t),
		indices, 
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	SDL_free(indices);

	// initialize vbos
	glGenVertexArrays(3, vao);
	for(int i=0; i<3; ++i) {
		glBindVertexArray(vao[i]);
		glBindBuffer(GL_ARRAY_BUFFER, plotter->getVBO(i));
		glEnableVertexAttribArray(aPosition);
		glEnableVertexAttribArray(aUV);
		glEnableVertexAttribArray(aColor);
		glVertexAttribPointer(aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
		glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)12);
		glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLvoid*)20);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuf);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glDisableVertexAttribArray(aPosition);
		glDisableVertexAttribArray(aUV);
		glDisableVertexAttribArray(aColor);
		
	}
	
}

SpritePlotter::~SpritePlotter() {
	glDeleteBuffers(1, &elementBuf);
	glDeleteVertexArrays(3, vao);
}

void SpritePlotter::begin(const Viewport& aView) {
	ASSERT(!isBound());
	count = 0;
	view = aView;
	shader.use();
	view.setMVP(uMVP);
}

void SpritePlotter::drawImage(ImageAsset *img, Vec2 pos, int frame, Color c, float z) {
	ASSERT(isBound());
	setTextureAtlas(img->texture);
	auto slice = nextSlice();
	FrameAsset *fr = img->frame(frame);

	pos -= fr->pivot;
	
	slice[0].set(pos,               z, fr->uv0, c);
	slice[1].set(pos+vec(0,fr->size.y),  z, fr->uv1, c);
	slice[2].set(pos+vec(fr->size.x, 0), z, fr->uv2, c);
	slice[3].set(pos+fr->size,    z, fr->uv3, c);

	++count;
}

void SpritePlotter::drawImage(ImageAsset *img, Vec2 pos, Vec2 u, int frame, Color c, float z) {
	ASSERT(isBound());
	setTextureAtlas(img->texture);
	auto slice = nextSlice();
	FrameAsset *fr = img->frame(frame);

	Vec2 p0 = -fr->pivot;
	Vec2 p1 = p0 + vec(0, fr->size.y);
	Vec2 p2 = p0 + vec(fr->size.x, 0);
	Vec2 p3 = p0 + fr->size;

	slice[0].set(pos+cmul(p0,u), z, fr->uv0, c);
	slice[1].set(pos+cmul(p1,u), z, fr->uv1, c);
	slice[2].set(pos+cmul(p2,u), z, fr->uv2, c);
	slice[3].set(pos+cmul(p3,u), z, fr->uv3, c);

	++count;
}

void SpritePlotter::drawImage(ImageAsset *img, const AffineMatrix& xform, int frame, Color c, float z) {
	ASSERT(isBound());
	setTextureAtlas(img->texture);
	auto slice = nextSlice();
	FrameAsset *fr = img->frame(frame);

	Vec2 p0 = -fr->pivot;
	Vec2 p1 = p0 + vec(0, fr->size.y);
	Vec2 p2 = p0 + vec(fr->size.x, 0);
	Vec2 p3 = p0 + fr->size;
	
	slice[0].set(xform.transformPoint(p0), z, fr->uv0, c);
	slice[1].set(xform.transformPoint(p1), z, fr->uv1, c);
	slice[2].set(xform.transformPoint(p2), z, fr->uv2, c);
	slice[3].set(xform.transformPoint(p3), z, fr->uv3, c);

	++count;	
}

void SpritePlotter::drawQuad(ImageAsset *img, Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, int frame, Color c, float z) {
	ASSERT(isBound());
	setTextureAtlas(img->texture);
	auto slice = nextSlice();
	FrameAsset *fr = img->frame(frame);

	slice[0].set(p0, z, fr->uv0, c);
	slice[1].set(p1, z, fr->uv1, c);
	slice[2].set(p2, z, fr->uv2, c);
	slice[3].set(p3, z, fr->uv3, c);

	++count;	
}

#define UV_LABEL_SLOP (0.0001f)

void SpritePlotter::plotGlyph(const GlyphAsset& g, float x, float y, float z, float h, Color c) {
	if (count == capacity()) {
		commitBatch();
	}

	auto slice = nextSlice();
	float k = 1.f / workingTexture->w;
	Vec2 uv = k * vec((float)g.x, (float)g.y);
	float du = k * (g.advance-UV_LABEL_SLOP);
	float dv = k * h;

	slice[0].set(vec(x,y), z, uv, c);
	slice[1].set(vec(x,y+h), z, uv+vec(0,dv), c);
	slice[2].set(vec(x+g.advance,y), z, uv+vec(du,0), c);
	slice[3].set(vec(x+g.advance,y+h), z, uv+vec(du,dv), c);

	++count;
}

void SpritePlotter::drawLabel(FontAsset *font, Vec2 p, Color c, const char *msg, float z) {
	ASSERT(isBound());
	setTextureAtlas(&(font->texture));

	float px = p.x;
	float py = p.y;
	while(*msg) {
		if (*msg != '\n') {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph( g, px, py, z, (float) font->height, c);
			px += g.advance;
		} else {
			px = p.x;
			py += font->height;
		}
		++msg;
	}

}

void SpritePlotter::drawLabelCentered(FontAsset *font, Vec2 p, Color c, const char *msg, float z) {
	ASSERT(isBound());
	setTextureAtlas(&(font->texture) );
	float py = p.y;
	while(*msg) {
		int length;
		const char* next = font->measureLine(msg, &length);
		float px = p.x - (length>>1);
		while(msg != next) {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph( g, px, py, z, (float) font->height, c);
			px += g.advance;
			msg++;
		}
		if (*msg == '\n') {
			py += font->height;
			msg++;
		}
	}
}

void SpritePlotter::drawLabelRightJustified(FontAsset *font, Vec2 p, Color c, const char *msg, float z) {
	ASSERT(isBound());
	setTextureAtlas(&(font->texture) );
	float py = p.y;
	while(*msg) {
		int length;
		const char* next = font->measureLine(msg, &length);
		float px = p.x - length;
		while(msg != next) {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph( g, px, py, z, (float) font->height, c);
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

void SpritePlotter::drawTilemap(TilemapAsset *map, Vec2 position, float z) {
	ASSERT(isBound());

	// make sure the map is initialized
	map->init();
	
	Vec2 cs = view.size() / vec((float)map->tw, (float)map->th);
	int latticeW = floorToInt(ceilf(cs.x) + 1);
	int latticeH = floorToInt(ceilf(cs.y) + 1);

	Vec2 scroll = view.offset() - position;
	

	int vox = int(scroll.x/map->tw);
	int voy = int(scroll.y/map->th);
	
	Vec2 rem = vec(
		fmodf(scroll.x, (float)map->tw),
		fmodf(scroll.y, (float)map->th)
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
		if (rawX >= 0 && rawX < (int)map->mw && rawY >= 0 && rawY < (int)map->mh) {
			TileAsset coord = map->tileAt(rawX, rawY);
			if (coord.isDefined()) {
				Vec2 p = vec((float) x * map->tw, (float) y * map->th) 
					- vec(TILE_SLOP, TILE_SLOP) 
					- rem + view.offset();
				Vec2 uv = 
					(vec(map->tw * coord.x + TILE_SLOP, map->th * coord.y + TILE_SLOP))
					/ vec((float) map->tileAtlas.w, (float) map->tileAtlas.h);
				auto slice = nextSlice();

				slice[0].set(p, z, uv, rgba(0));
				slice[1].set(p+vec(0,th), z, uv+vec(0,uh), rgba(0));
				slice[2].set(p+vec(tw,0), z, uv+vec(uw,0), rgba(0));
				slice[3].set(p+vec(tw,th), z, uv+vec(uw,uh), rgba(0));

				if (++count == capacity()) {
					commitBatch();
				}
			}
		}
	}	
}

void SpritePlotter::flush() {
	ASSERT(isBound()); 
	if (count > 0) { 
		commitBatch(); 
	}
}

void SpritePlotter::end() {
	ASSERT(isBound());
	flush();
	count = -1;
	workingTexture = 0;
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void SpritePlotter::commitBatch() {
	ASSERT(count > 0);
	
	plotter->bufferData(count<<2);

	glBindVertexArray(vao[plotter->getCurrentArray()]);
	glDrawElements(GL_TRIANGLES, 6 * count, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
	
	plotter->swapBuffer();
	count = 0;
}

void SpritePlotter::setTextureAtlas(TextureAsset *texture) {
	bool atlasChange = texture != workingTexture;
	
	// emit a draw call if we're at SPRITE_CAPACITY of if the atlas is changing.
	// (assuming that count will be 0 if workingTexture is null)
	if (count == capacity() || (count > 0 && atlasChange)) {
		commitBatch();
	}

	// check if we need to bind a new texture
	if (atlasChange) {
		texture->bind();
		workingTexture = texture;		
	}
}




