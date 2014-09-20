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

#include "graphics.h"
#include "pools.h"

//------------------------------------------------------------------------------
// ASSETS

#define ASCII_BEGIN          32
#define ASCII_END            127


struct FrameAsset
{
	lpVec uv0, uv1,  // UV coordinates of the corners (necessary since the image
	      uv2, uv3,  // might have been transposed during packing)
	      pivot,     // post-trim pivot of the frame
	      size;      // post-trim size of the frame
	
	
};

struct ImageAsset
{
	TextureAsset* texture; // the texture onto which this image is packed
	FrameAsset *  frames;
	lpVec         size, pivot;
	int32_t       nframes;
	
	FrameAsset* frame(int i) {
		ASSERT(i >= 0 && i < nframes);
		return frames + i;
	}
	
};

//------------------------------------------------------------------------------

struct TileAsset
{
	
	uint8_t x, y; // Atlas Cell Location
	
	bool isDefined() const { return x != 0xff; }
	
};

struct TilemapAsset
{
	
	TileAsset*   data;           // NULL when uninitialized
	void*        compressedData; // zlib compressed tilemap buffer
	int32_t      tw, th,         // the size of the individual tiles
	             mw, mh;         // the size of the tilemap
	uint32_t     compressedSize; // the byte-length of the compressed buffer
	TextureAsset tileAtlas;      // a texture-atlas of all the tiles
	
	bool initialized() const { return data != 0; }
	lpVec tileSize() const { return vec((lpFloat)tw,(lpFloat)th); }
	lpVec mapSize() const { return vec((lpFloat)mw,(lpFloat)mh); }
	TileAsset tileAt(int x, int y) const;
	
	void init();
	void release();
	
	void reload();
	void clearTile(int x, int y);
	
};

//------------------------------------------------------------------------------

struct GlyphAsset
{
	
	int32_t x, y,     // texel position of this glyph
	        advance;  // pixel offset to the next character
	
};

struct FontAsset
{
	
	int32_t      height;                        // line-height of the font
	GlyphAsset   glyphs[ASCII_END-ASCII_BEGIN]; // individual glyph metrics
	TextureAsset texture;                       // character texture-atlas
	
	GlyphAsset getGlyph(const char c) {
		ASSERT(c >= ASCII_BEGIN && c < ASCII_END);
		return glyphs[c-ASCII_BEGIN];
	}
	
	const char* measureLine(const char *msg, int *outLength) {
		*outLength = 0;
		while(*msg && *msg != '\n') {
			*outLength += getGlyph(*msg).advance;
			++msg;
		}
		return msg;
	}
	
};

//------------------------------------------------------------------------------
// SPRITE PLOTTER

// This object can render lots of sprites in a small number of batched draw calls
// by coalescing adjacent draws into larger logical draws.
// TODO: perform batch-level clipping?

class SpritePlotter {
private:
	Plotter *plotter;
	int count;
	
	Viewport view;
	Shader shader;
	
	GLuint uMVP;
	GLuint uAtlas;
	GLuint aPosition;
	GLuint aUV;
	GLuint aColor;
	GLuint aTint;
	
	GLuint vao[3];
	GLuint elementBuf;
	
	TextureAsset *workingTexture;

public:
	SpritePlotter(Plotter *plotter);
	~SpritePlotter();

	int capacity() const { return plotter->getCapacity()>>2; }
	bool isBound() const { return count >= 0; }
	const Viewport& viewport() const { return view; }

	// Call this method to initialize the graphics context state.  Asserts that the plotter
	// is already bound (in case you're coalescing with other plotters) and state e.g. blending are
	// enabled.  Any additional state changes can be set *after* this function but *before*
	// issuing any draw calls.
	void begin(const Viewport& view);

	// Draw the given image.  Will potentially cause a draw call to actually be emitted
	// to the graphics device if: (i) the buffer has reached capacity or (ii) the texture 
	// atlas has changed.  Color transforms *can* be batched, because they are encoded
	// in the vertices, not in shader uniforms.
	void drawImage(ImageAsset *image, lpVec position, int frame=0, Color color=rgba(0), Color tint=rgba(0xffffffff));
	void drawImage(ImageAsset *image, lpVec position, lpVec u, int frame=0, Color color=rgba(0), Color tint=rgba(0xffffffff));
	void drawImage(ImageAsset *image, const lpMatrix& xform, int frame=0, Color color=rgba(0), Color tint=rgba(0xffffffff));
	void drawQuad(ImageAsset *image, lpVec p0, lpVec p1, lpVec p2, lpVec p3, int frame=0, Color color=rgba(0), Color tint=rgba(0xffffffff));
	void drawLabel(FontAsset *font, lpVec p, Color c, const char *msg, Color tint=rgba(0xffffffff));
	void drawLabelCentered(FontAsset *font, lpVec p, Color c, const char *msg, Color tint=rgba(0xffffffff));
	void drawLabelRightJustified(FontAsset *font, lpVec p, Color c, const char *msg, Color tint=rgba(0xffffffff));
	void drawTilemap(TilemapAsset *map, lpVec position=vec(0,0), Color tint=rgba(0xffffffff));

	// if you want to monkey with the global rendering state (e.g. change blending settings)
	// you need to flush the render queue first.
	void flush();

	// Commit the current draw queue and return the graphics context state to it's
	// canonical form, to play nice with other renderers.
	void end();

private:
	Vertex *nextSlice() { return plotter->getVertex(count<<2); }
	void setTextureAtlas(TextureAsset* texture);
	void commitBatch();
	void plotGlyph(const GlyphAsset& g, lpFloat x, lpFloat y, lpFloat h, Color c, Color t);

};

//------------------------------------------------------------------------------
// SPRITE BATCH
//
// A special object that can be used in a Batch<T> (see pools.h) to efficiently
// render a collection of sprites.  (Could be specialized, e.g. if you know all
// your sprites aren't scaled, rotated, tinted, animated, and so forth).

struct Sprite
{
	ImageAsset *image;
	lpMatrix xform;
	int frame;
	Color color;
	Color tint;
	
	// convenience ctors
	Sprite() : image(0), xform(matIdentity()), frame(0), color(rgba(0)), tint(rgba(0xffffffff)) {}
	Sprite(ImageAsset *img) : image(img), xform(matIdentity()), frame(0), color(rgba(0)), tint(rgba(0xffffffff)) {}
	
	Sprite(ImageAsset *img, lpVec pos) : image(img), xform(matTranslation(pos)), frame(0), color(rgba(0)), tint(rgba(0xffffffff)) {}
	Sprite(ImageAsset *img, lpVec pos, int fr) : image(img), xform(matTranslation(pos)), frame(fr), color(rgba(0)), tint(rgba(0xffffffff)) {}
	Sprite(ImageAsset *img, lpVec pos, Color col) : image(img), xform(matTranslation(pos)), frame(0), color(col), tint(rgba(0xffffffff)) {}
	Sprite(ImageAsset *img, lpVec pos, Color col, Color tint) : image(img), xform(matTranslation(pos)), frame(0), color(col), tint(tint) {}
	Sprite(ImageAsset *img, lpVec pos, int fr, Color col) : image(img), xform(matTranslation(pos)), frame(fr), color(col), tint(rgba(0xffffffff)) {}
	Sprite(ImageAsset *img, lpVec pos, int fr, Color col, Color tint) : image(img), xform(matTranslation(pos)), frame(fr), color(col), tint(tint) {}

	Sprite(ImageAsset *img, lpMatrix mat) : image(img), xform(mat), frame(0), color(rgba(0)), tint(rgba(0xffffffff)) {}
	Sprite(ImageAsset *img, lpMatrix mat, int fr) : image(img), xform(mat), frame(fr), color(rgba(0)), tint(rgba(0xffffffff)) {}
	Sprite(ImageAsset *img, lpMatrix mat, Color col) : image(img), xform(mat), frame(0), color(col), tint(rgba(0xffffffff)) {}
	Sprite(ImageAsset *img, lpMatrix mat, Color col, Color tint) : image(img), xform(mat), frame(0), color(col), tint(tint) {}
	Sprite(ImageAsset *img, lpMatrix mat, int fr, Color col) : image(img), xform(mat), frame(fr), color(col), tint(rgba(0xffffffff)) {}
	Sprite(ImageAsset *img, lpMatrix mat, int fr, Color col, Color tint) : image(img), xform(mat), frame(fr), color(col), tint(tint) {}
	
	inline lpVec position() const { return xform.t; }
	inline void setPosition(lpVec pos) { xform.t = pos; }
	
	inline lpVec attitude() const { return xform.u; }
	inline void setAttitude(lpVec att) { xform.u = att; xform.v = lpVec(-att.y, att.x); }
	
	inline void draw(SpritePlotter* plotter)
	{
		plotter->drawImage(image, xform, frame, color, tint);
	}
	
	inline void drawClipped(SpritePlotter *plotter)
	{
		auto& view = plotter->viewport();
		auto sz = xform.transformVector(image->size);
		auto pad = lpAbs(sz.x + sz.y);
		if (view.contains(xform.t, pad)) {
			plotter->drawImage(image, xform, frame, color, tint);
		}
	}
};

typedef BatchPool<Sprite> SpriteBatch;
typedef BatchHandle<Sprite> SpriteHandle;



