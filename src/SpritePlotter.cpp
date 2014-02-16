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

SpritePlotter::SpritePlotter(BasicPlotterRef aPlotter) : 
plotter(aPlotter), 
count(-1), 
workingTexture(0) 
{
	capacity = plotter.capacity() >> 2;

	// setup element array buffer
	uint16_t indices[6 * capacity];
	for(int i=0; i<capacity; ++i) {
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
		6 * capacity * sizeof(uint16_t), 
		indices, 
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return ;
}

SpritePlotter::~SpritePlotter() {
	glDeleteBuffers(1, &elementBuf); 
}

void SpritePlotter::begin(const Viewport& view) {
	ASSERT(!plotter.isBound());
	ASSERT(!isBound());
	count = 0;
	plotter.begin(view);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuf);
}

void SpritePlotter::drawImage(ImageAsset *img, vec2 pos, int frame, Color c) {
	ASSERT(isBound());
	setTextureAtlas(img->texture);
	auto slice = nextSlice();
	FrameAsset *fr = img->frame(frame);

	pos -= fr->pivot();
	
	slice[0].set(pos,               fr->uv0, c);
	slice[1].set(pos+vec(0,fr->h),  fr->uv1, c);
	slice[2].set(pos+vec(fr->w, 0), fr->uv2, c);
	slice[3].set(pos+fr->size(),    fr->uv3, c);

	++count;
}

void SpritePlotter::drawImage(ImageAsset *img, vec2 pos, vec2 u, int frame, Color c) {
	ASSERT(isBound());
	setTextureAtlas(img->texture);
	auto slice = nextSlice();
	FrameAsset *fr = img->frame(frame);

	vec2 p0 = -fr->pivot();
	vec2 p1 = p0 + vec(0, fr->h);
	vec2 p2 = p0 + vec(fr->w, 0);
	vec2 p3 = p0 + fr->size();

	slice[0].set(pos+cmul(p0,u), fr->uv0, c);
	slice[1].set(pos+cmul(p1,u), fr->uv1, c);
	slice[2].set(pos+cmul(p2,u), fr->uv2, c);
	slice[3].set(pos+cmul(p3,u), fr->uv3, c);

	++count;
}

void SpritePlotter::drawImage(ImageAsset *img, const AffineMatrix& xform, int frame, Color c) {
	ASSERT(isBound());
	setTextureAtlas(img->texture);
	auto slice = nextSlice();
	FrameAsset *fr = img->frame(frame);

	vec2 p0 = -fr->pivot();
	vec2 p1 = p0 + vec(0, fr->h);
	vec2 p2 = p0 + vec(fr->w, 0);
	vec2 p3 = p0 + fr->size();
	
	slice[0].set(xform.transformPoint(p0), fr->uv0, c);
	slice[1].set(xform.transformPoint(p1), fr->uv1, c);
	slice[2].set(xform.transformPoint(p2), fr->uv2, c);
	slice[3].set(xform.transformPoint(p3), fr->uv3, c);

	++count;	
}

void SpritePlotter::drawImage(ImageAsset *img, const mat4f& xform, int frame, Color c) {
	ASSERT(isBound());
	setTextureAtlas(img->texture);
	auto slice = nextSlice();
	FrameAsset *fr = img->frame(frame);

	vec3f p0 = -vec3f(fr->px, fr->py, 0);
	vec3f p1 = p0 + vec3f(0, fr->h, 0);
	vec3f p2 = p0 + vec3f(fr->w, 0, 0);
	vec3f p3 = p0 + vec3f(fr->w, fr->h, 0);

	slice[0].set(transformPoint(xform, p0).xy(), fr->uv0, c);
	slice[1].set(transformPoint(xform, p1).xy(), fr->uv1, c);
	slice[2].set(transformPoint(xform, p2).xy(), fr->uv2, c);
	slice[3].set(transformPoint(xform, p3).xy(), fr->uv3, c);

	++count;	
}

void SpritePlotter::drawQuad(ImageAsset *img, vec2 p0, vec2 p1, vec2 p2, vec2 p3, int frame, Color c) {
	ASSERT(isBound());
	setTextureAtlas(img->texture);
	auto slice = nextSlice();
	FrameAsset *fr = img->frame(frame);

	slice[0].set(p0, fr->uv0, c);
	slice[1].set(p1, fr->uv1, c);
	slice[2].set(p2, fr->uv2, c);
	slice[3].set(p3, fr->uv3, c);

	++count;	
}

#define UV_LABEL_SLOP (0.0001f)

void SpritePlotter::plotGlyph(const GlyphAsset& g, float x, float y, float h, Color c) {
	if (count == capacity) {
		commitBatch();
	}

	auto slice = nextSlice();
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

void SpritePlotter::drawLabel(FontAsset *font, vec2 p, Color c, const char *msg) {
	ASSERT(isBound());
	setTextureAtlas(&(font->texture));

	float px = p.x;
	float py = p.y;
	while(*msg) {
		if (*msg != '\n') {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph( g, px, py, font->height, c);
			px += g.advance;
		} else {
			px = p.x;
			py += font->height;
		}
		++msg;
	}

}

void SpritePlotter::drawLabelCentered(FontAsset *font, vec2 p, Color c, const char *msg) {
	ASSERT(isBound());
	setTextureAtlas(&(font->texture) );
	float py = p.y;
	while(*msg) {
		int length;
		const char* next = font->measureLine(msg, &length);
		float px = p.x - (length>>1);
		while(msg != next) {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph( g, px, py, font->height, c);
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

void SpritePlotter::drawTilemap(TilemapAsset *map, vec2 position) {
	ASSERT(isBound());

	// make sure the map is initialized
	map->init();
	auto view = plotter.view();
	
	vec2 cs = view->size() / vec(map->tw, map->th);
	int latticeW = ceilf(cs.x) + 1;
	int latticeH = ceilf(cs.y) + 1;

	vec2 scroll = view->offset() - position;
	

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
					- rem + view->offset();
				vec2 uv = 
					(vec(map->tw * coord.x, map->th * coord.y) + vec(TILE_SLOP, TILE_SLOP))
					/ vec(map->tileAtlas.w, map->tileAtlas.h);
				auto slice = nextSlice();

				slice[0].set(p, uv, rgba(0));
				slice[1].set(p+vec(0,th), uv+vec(0,uh), rgba(0));
				slice[2].set(p+vec(tw,0), uv+vec(uw,0), rgba(0));
				slice[3].set(p+vec(tw,th), uv+vec(uw,uh), rgba(0));

				if (++count == capacity) {
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
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	plotter.end();
}

void SpritePlotter::commitBatch() {
	ASSERT(count > 0);
	plotter.commit(count<<2);
	glDrawElements(GL_TRIANGLES, 6 * count, GL_UNSIGNED_SHORT, 0);
	count = 0;
}

void SpritePlotter::setTextureAtlas(TextureAsset *texture) {
	bool atlasChange = texture != workingTexture;
	
	// emit a draw call if we're at SPRITE_CAPACITY of if the atlas is changing.
	// (assuming that count will be 0 if workingTexture is null)
	if (count == capacity || (count > 0 && atlasChange)) {
		commitBatch();
	}

	// check if we need to bind a new texture
	if (atlasChange) {
		texture->bind();
		workingTexture = texture;		
	}
}




