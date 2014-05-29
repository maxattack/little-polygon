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

//------------------------------------------------------------------------------
// ASSETS

#define ASCII_BEGIN          32
#define ASCII_END            127


struct FrameAsset
{
	Vec2 uv0, uv1,  // UV coordinates of the corners (necessary since the image
	     uv2, uv3,  // might have been transposed during packing)
	     pivot,    // post-trim pivot of the frame
	     size;      // post-trim size of the frame
	
	
};

struct ImageAsset
{
	TextureAsset* texture; // the texture onto which this image is packed
	FrameAsset *frames;
	Vec2 size, pivot;
	uint32_t nframes;
	
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
	uint32_t     tw, th,         // the size of the individual tiles
	mw, mh,         // the size of the tilemap
	compressedSize; // the byte-length of the compressed buffer
	TextureAsset tileAtlas;      // a texture-atlas of all the tiles
	
	bool initialized() const { return data != 0; }
	Vec2 tileSize() const { return vec((float)tw,(float)th); }
	Vec2 mapSize() const { return vec((float)mw,(float)mh); }
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
// FORWARD DECLARATIONS

class SpritePlotter;

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
	
	
	GLuint vao[3];
	GLuint elementBuf;
	
	TextureAsset *workingTexture;

public:
	SpritePlotter(Plotter *plotter);
	~SpritePlotter();

	int capacity() const { return plotter->getCapacity()>>2; }
	bool isBound() const { return count >= 0; }

	// Call this method to initialize the graphics context state.  Asserts that the plotter
	// is already bound (in case you're coalescing with other plotters) and state e.g. blending are
	// enabled.  Any additional state changes can be set *after* this function but *before*
	// issuing any draw calls.
	void begin(const Viewport& view);

	// Draw the given image.  Will potentially cause a draw call to actually be emitted
	// to the graphics device if: (i) the buffer has reached capacity or (ii) the texture 
	// atlas has changed.  Color transforms *can* be batched, because they are encoded
	// in the vertices, not in shader uniforms.
	void drawImage(ImageAsset *image, Vec2 position, int frame=0, Color color=rgba(0), float z=0);
	void drawImage(ImageAsset *image, Vec2 position, Vec2 u, int frame=0, Color color=rgba(0), float z=0);
	void drawImage(ImageAsset *image, const AffineMatrix& xform, int frame=0, Color color=rgba(0), float z=0);
	void drawQuad(ImageAsset *image, Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, int frame=0, Color color=rgba(0), float z=0);
	void drawLabel(FontAsset *font, Vec2 p, Color c, const char *msg, float z=0);
	void drawLabelCentered(FontAsset *font, Vec2 p, Color c, const char *msg, float z=0);
	void drawLabelRightJustified(FontAsset *font, Vec2 p, Color c, const char *msg, float z=0);
	void drawTilemap(TilemapAsset *map, Vec2 position=vec(0,0), float z=0);

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
	void plotGlyph(const GlyphAsset& g, float x, float y, float z, float h, Color c);

};
