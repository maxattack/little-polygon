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
in vec2 aPosition;
in vec2 aUv;
in vec4 aColor;
in vec4 aTint;
out vec2 uv;
out vec4 color;
out vec4 tint;

void main()
{
	gl_Position = mvp * vec4(aPosition, 0, 1.0);
	color = aColor;
	uv = aUv;
	tint = aTint;
}

);

const GLchar SPRITE_FRAG[] = GLSL(

uniform sampler2D atlas;
in vec2 uv;
in vec4 color;
in vec4 tint;
out vec4 outColor;

void main()
{
	vec4 baseColor = texture(atlas, uv);
	outColor = tint * vec4(mix(baseColor.rgb, color.rgb, color.a), baseColor.a);
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
	aTint = shader.attribLocation("aTint");
	
	// setup element array buffer
	Array<uint16_t> indices(6 * capacity());
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
		indices.ptr(),
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// initialize vbos
	glGenVertexArrays(3, vao);
	for(int i=0; i<3; ++i) {
		glBindVertexArray(vao[i]);
		glBindBuffer(GL_ARRAY_BUFFER, plotter->getVBO(i));
		glEnableVertexAttribArray(aPosition);
		glEnableVertexAttribArray(aUV);
		glEnableVertexAttribArray(aColor);
		glEnableVertexAttribArray(aTint);
		glVertexAttribPointer(aPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
		glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)8);
		glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLvoid*)16);
		glVertexAttribPointer(aTint, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLvoid*)20);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuf);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glDisableVertexAttribArray(aPosition);
		glDisableVertexAttribArray(aUV);
		glDisableVertexAttribArray(aColor);
		glDisableVertexAttribArray(aTint);
	}
	
}

SpritePlotter::~SpritePlotter()
{
	glDeleteBuffers(1, &elementBuf);
	glDeleteVertexArrays(3, vao);
}

void SpritePlotter::begin(const Viewport& aView)
{
	ASSERT(!isBound());
	count = 0;
	view = aView;
	shader.use();
	view.setMVP(uMVP);
}

void SpritePlotter::drawImage(ImageAsset *img, lpVec pos, int frame, Color c, Color tint)
{
	auto *fr = img->frame(frame);
	pos -= fr->pivot;
	drawQuad(
		img,
		pos,
		pos+vec(0,fr->size.y),
		pos+vec(fr->size.x, 0),
		pos+fr->size,
		frame, c, tint
	);
}

void SpritePlotter::drawImage(ImageAsset *img, lpVec pos, lpVec u, int frame, Color c, Color tint)
{
	auto *fr = img->frame(frame);
	lpVec p0 = -fr->pivot;
	drawQuad(
		img,
		pos+cmul(p0, u),
		pos+cmul(p0 + vec(0, fr->size.y), u),
		pos+cmul(p0 + vec(fr->size.x, 0), u),
		pos+cmul(p0 + fr->size, u),
		frame, c, tint
	);
}

void SpritePlotter::drawImage(ImageAsset *img, const lpMatrix& xform, int frame, Color c, Color tint)
{
	auto *fr = img->frame(frame);
	lpVec p0 = -fr->pivot;
	drawQuad(
		img,
		xform.transformPoint(p0),
		xform.transformPoint(p0 + vec(0, fr->size.y)),
		xform.transformPoint(p0 + vec(fr->size.x, 0)),
		xform.transformPoint(p0 + fr->size),
		frame, c, tint
	);
}

inline float min(float a, float b) { return a < b ? a : b;  }
inline float max(float a, float b) { return a > b ? a : b;  }

inline float min4(float a, float b, float c, float d)
{
	return min(min(a,b), min(c,d));
}

inline float max4(float a, float b, float c, float d)
{
	return max(max(a,b), max(c,d));
}

void SpritePlotter::drawQuad(ImageAsset *img, lpVec p0, lpVec p1, lpVec p2, lpVec p3, int frame, Color c, Color tint)
{
	ASSERT(isBound());
	
	// OBB Test (faster impl?)
	auto min = vec(min4(p0.x, p1.x, p2.x, p3.x), min4(p0.y, p1.y, p2.y, p3.y));
	auto max = vec(max4(p0.x, p1.x, p2.x, p3.x), max4(p0.y, p1.y, p2.y, p3.y));
	auto halfSize = 0.5f * (max - min);
	auto center = min + halfSize;
	auto offset = center - view.center();
	auto sz = halfSize + view.halfSize();
	
	if (lpAbs(offset.x) < sz.x && lpAbs(offset.y) < sz.y) {
	
		setTextureAtlas(img->texture);
		auto slice = nextSlice();
		FrameAsset *fr = img->frame(frame);

		slice[0].set(p0, fr->uv0, c, tint);
		slice[1].set(p1, fr->uv1, c, tint);
		slice[2].set(p2, fr->uv2, c, tint);
		slice[3].set(p3, fr->uv3, c, tint);

		++count;
	}
}

#define UV_LABEL_SLOP (0.0001f)

void SpritePlotter::plotGlyph(const GlyphAsset& g, lpFloat x, lpFloat y, lpFloat h, Color c, Color t)
{
	if (count == capacity()) {
		commitBatch();
	}

	auto slice = nextSlice();
	lpFloat k = 1.f / workingTexture->w;
	lpVec uv = k * vec((lpFloat)g.x, (lpFloat)g.y);
	lpFloat du = k * (g.advance-UV_LABEL_SLOP);
	lpFloat dv = k * h;

	uv.x += UV_LABEL_SLOP;
	uv.y += UV_LABEL_SLOP;
	du -= 2.0f * UV_LABEL_SLOP;
	dv -= 2.0f * UV_LABEL_SLOP;
	
	slice[0].set(vec(x,y), uv, c, t);
	slice[1].set(vec(x,y+h), uv+vec(0,dv), c, t);
	slice[2].set(vec(x+g.advance,y), uv+vec(du,0), c, t);
	slice[3].set(vec(x+g.advance,y+h), uv+vec(du,dv), c, t);

	++count;
}

void SpritePlotter::drawLabel(FontAsset *font, lpVec p, Color c, const char *msg, Color tint)
{
	ASSERT(isBound());
	setTextureAtlas(&(font->texture));

	lpFloat px = p.x;
	lpFloat py = p.y;
	while(*msg) {
		if (*msg != '\n') {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph( g, px, py, (lpFloat) font->height, c, tint);
			px += g.advance;
		} else {
			px = p.x;
			py += font->height;
		}
		++msg;
	}

}

void SpritePlotter::drawLabelCentered(FontAsset *font, lpVec p, Color c, const char *msg, Color tint)
{
	ASSERT(isBound());
	setTextureAtlas(&(font->texture) );
	lpFloat py = p.y;
	while(*msg) {
		int length;
		const char* next = font->measureLine(msg, &length);
		lpFloat px = p.x - (length>>1);
		while(msg != next) {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph( g, px, py, (lpFloat) font->height, c, tint);
			px += g.advance;
			msg++;
		}
		if (*msg == '\n') {
			py += font->height;
			msg++;
		}
	}
}

void SpritePlotter::drawLabelRightJustified(FontAsset *font, lpVec p, Color c, const char *msg, Color tint)
{
	ASSERT(isBound());
	setTextureAtlas(&(font->texture) );
	lpFloat py = p.y;
	while(*msg) {
		int length;
		const char* next = font->measureLine(msg, &length);
		lpFloat px = p.x - length;
		while(msg != next) {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph( g, px, py, (lpFloat) font->height, c, tint);
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

void SpritePlotter::drawTilemap(TilemapAsset *map, lpVec position, Color tint)
{
	ASSERT(isBound());

	// make sure the map is initialized
	map->init();
	
	lpVec cs = view.size() / vec((lpFloat)map->tw, (lpFloat)map->th);
	int latticeW = floorToInt(lpCeil(cs.x) + 1);
	int latticeH = floorToInt(lpCeil(cs.y) + 1);

	lpVec scroll = view.offset() - position;
	

	int vox = int(scroll.x/map->tw);
	int voy = int(scroll.y/map->th);
	
	lpVec rem = vec(
		lpMod(scroll.x, (lpFloat)map->tw),
		lpMod(scroll.y, (lpFloat)map->th)
	);
	setTextureAtlas(&map->tileAtlas);

	lpFloat tw = map->tw + TILE_SLOP + TILE_SLOP;
	lpFloat th = map->th + TILE_SLOP + TILE_SLOP;
	lpFloat uw = (map->tw - TILE_SLOP - TILE_SLOP) / lpFloat(map->tileAtlas.w);
	lpFloat uh = (map->th - TILE_SLOP - TILE_SLOP) / lpFloat(map->tileAtlas.h);
	for(int y=0; y<latticeH; ++y)
	for(int x=0; x<latticeW; ++x) {
		int rawX = x+vox;
		int rawY = y+voy;
		if (rawX >= 0 && rawX < (int)map->mw && rawY >= 0 && rawY < (int)map->mh) {
			TileAsset coord = map->tileAt(rawX, rawY);
			if (coord.isDefined()) {
				lpVec p = vec((lpFloat) x * map->tw, (lpFloat) y * map->th) 
					- vec(TILE_SLOP, TILE_SLOP) 
					- rem + view.offset();
				lpVec uv = 
					(vec(map->tw * coord.x + TILE_SLOP, map->th * coord.y + TILE_SLOP))
					/ vec((lpFloat) map->tileAtlas.w, (lpFloat) map->tileAtlas.h);
				auto slice = nextSlice();

				slice[0].set(p, uv, rgba(0), tint);
				slice[1].set(p+vec(0,th),  uv+vec(0,uh),  rgba(0), tint);
				slice[2].set(p+vec(tw,0),  uv+vec(uw,0),  rgba(0), tint);
				slice[3].set(p+vec(tw,th), uv+vec(uw,uh), rgba(0), tint);

				if (++count == capacity()) {
					commitBatch();
				}
			}
		}
	}	
}

void SpritePlotter::flush()
{
	ASSERT(isBound()); 
	if (count > 0) { 
		commitBatch(); 
	}
}

void SpritePlotter::end()
{
	ASSERT(isBound());
	flush();
	count = -1;
	workingTexture = 0;
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void SpritePlotter::commitBatch()
{
	ASSERT(count > 0);
	
	plotter->bufferData(count<<2);

	glBindVertexArray(vao[plotter->getCurrentArray()]);
	glDrawElements(GL_TRIANGLES, 6 * count, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
	
	plotter->swapBuffer();
	count = 0;
}

void SpritePlotter::setTextureAtlas(TextureAsset *texture)
{
	bool atlasChange = texture != workingTexture;
	
	// emit a draw call if we're at capacity or if the atlas is changing.
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




