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

#include "assets.h"
#include "graphics.h"
#include "pools.h"

//------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//------------------------------------------------------------------------------

class SpritePlotter;
class SpriteBatch;
class Sprite;

//------------------------------------------------------------------------------
// SPRITE PLOTTER
//------------------------------------------------------------------------------

// This object can render lots of sprites in a small number of batched draw calls
// by coalescing adjacent draws into larger logical draws.
// TODO: perform batch-level clipping?

class SpritePlotter {
private:
	BasicPlotter* plotter;
	int capacity;
	int count;
	GLuint elementBuf;
	TextureAsset *workingTexture;

public:
	SpritePlotter(BasicPlotter* plotter);
	~SpritePlotter();

	bool isBound() const { return count >= 0; }
	BasicPlotter* getPlotter() { return plotter; }

	// Call this method to initialize the graphics context state.  Asserts that the plotter
	// is already bound (in case you're coalescing with other plotters) and state e.g. blending are
	// enabled.  Any additional state changes can be set *after* this function but *before*
	// issuing any draw calls.
	void begin(const Viewport& view, GLuint program=0);

	// Draw the given image.  Will potentially cause a draw call to actually be emitted
	// to the graphics device if: (i) the buffer has reached capacity or (ii) the texture 
	// atlas has changed.  Color transforms *can* be batched, because they are encoded
	// in the vertices, not in shader uniforms.
	void drawImage(ImageAsset *image, vec2 position, int frame=0, Color color=rgba(0), float z=0);
	void drawImage(ImageAsset *image, vec2 position, vec2 u, int frame=0, Color color=rgba(0), float z=0);
	void drawImage(ImageAsset *image, const AffineMatrix& xform, int frame=0, Color color=rgba(0), float z=0);
	void drawImage(ImageAsset *image, const mat4f& xform, int frame=0, Color color=rgba(0), float z=0);
	void drawQuad(ImageAsset *image, vec2 p0, vec2 p1, vec2 p2, vec2 p3, int frame=0, Color color=rgba(0), float z=0);
	void drawLabel(FontAsset *font, vec2 p, Color c, const char *msg, float z=0);
	void drawLabelCentered(FontAsset *font, vec2 p, Color c, const char *msg, float z=0);
	void drawLabelRightJustified(FontAsset *font, vec2 p, Color c, const char *msg, float z=0);
	void drawTilemap(TilemapAsset *map, vec2 position=vec(0,0), float z=0);

	// if you want to monkey with the global rendering state (e.g. change blending settings)
	// you need to flush the render queue first.
	void flush();

	// Commit the current draw queue and return the graphics context state to it's
	// canonical form, to play nice with other renderers.
	void end();

private:
	BasicVertex *nextSlice() { return plotter->getVertex(count<<2); }
	void setTextureAtlas(TextureAsset* texture);
	void commitBatch();
	void plotGlyph(const GlyphAsset& g, float x, float y, float z, float h, Color c);

};

//------------------------------------------------------------------------------
// SPRITE BATCH - storage structure for saving a reuseable
// command-buffer of draw calls (decoupled from plotter so that
// calls can be freely mixed with other systems).
//------------------------------------------------------------------------------

class Sprite {
friend class SpriteBatch;
private:
	SpriteBatch *mBatch;
	int mLayer, mIndex;

	void clear();
	
public:
	Sprite(SpriteBatch *batch, int layer, int index);
	
	SpriteBatch* batch() const { return mBatch; }
	
	const AffineMatrix& transform() const;
	void setTransform(const AffineMatrix& xform);
	
	ImageAsset* image() const;
	void setImage(ImageAsset *image);
	
	int layer() const { return mLayer; }
	void setLayer(int i);

	Color fill() const;
	void setFill(Color aFill);
	
	int frame() const;
	void setFrame(int frame);

	void release();
};

class SpriteBatch {
friend class Sprite;
private:
	struct DrawCall {
		Sprite *sprite;
		ImageAsset *img;
		AffineMatrix xform;
		Color color;
		int frame;
		float depth;
	};
	
	typedef CompactPool<DrawCall> Layer;
	
	int mLayerCount;
	Layer* mLayers;
	DynamicPool<Sprite> mSprites;
	
public:
	SpriteBatch(int nlayers);
	~SpriteBatch();
	
	int layerCount() const { return mLayerCount; }
	
	Sprite *addSprite(
		int layer,
		ImageAsset *image,
		const AffineMatrix& xform,
		int frame=0,
		Color c=rgba(0),
		float depth=0
	);
	
	void draw(SpritePlotter *plotter);

	void clear();
	
};

