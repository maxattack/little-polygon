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

#include "littlepolygon_assets.h"
#include "littlepolygon_math.h"

//------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//------------------------------------------------------------------------------

struct SpritePlotter;
struct SpriteBatch;
struct Sprite;

//------------------------------------------------------------------------------
// SPRITE PLOTTER
//------------------------------------------------------------------------------

// This object can render lots of sprites in a small number of batched draw calls
// by coalescing adjacent draws into larger logical draws.
// TODO: perform batch-level clipping?

SpritePlotter *createSpritePlotter(int capacity=64);

class SpritePlotterRef {
private:
	SpritePlotter *context;

public:
	SpritePlotterRef() {}
	SpritePlotterRef(SpritePlotter *aContext) : context(aContext) {}

	operator SpritePlotter*() { return context; }
	operator bool() const { return context; }

	void destroy();

	bool bound() const;
	vec2 canvasSize() const;
	vec2 canvasScroll() const;

	// Call this method to initialize the graphics context state.  Coordinates are
	// set to a orthogonal projection matrix, and some basic settings like blending are
	// enabled.  Any additional state changes can be set *after* this function but *before*
	// issuing any draw calls.
	void begin(vec2 canvasSize, vec2 scrolling=vec(0,0));

	// Draw the given image.  Will potentially cause a draw call to actually be emitted
	// to the graphics device if: (i) the buffer has reached capacity or (ii) the texture 
	// atlas has changed.  Color transforms *can* be batched, because they are encoded
	// in the vertices, not in shader uniforms.
	void drawImage(ImageAsset *image, vec2 position, int frame=0, Color color=rgba(0));
	void drawImage(ImageAsset *image, vec2 position, vec2 u, int frame=0, Color color=rgba(0));
	void drawImage(ImageAsset *image, const AffineMatrix& xform, int frame=0, Color color=rgba(0));
	void drawImage(ImageAsset *image, const mat4f& xform, int frame=0, Color color=rgba(0));
	void drawQuad(ImageAsset *image, vec2 p0, vec2 p1, vec2 p2, vec2 p3, int frame=0, Color color=rgba(0));
	void drawLabel(FontAsset *font, vec2 p, Color c, const char *msg);
	void drawLabelCentered(FontAsset *font, vec2 p, Color c, const char *msg);
	void drawTilemap(TilemapAsset *map, vec2 position=vec(0,0));	

	// if you want to monkey with the global rendering state (e.g. change blending settings)
	// you need to flush the render queue first.
	void flush();

	// Commit the current draw queue and return the graphics context state to it's
	// canonical form, to play nice with other renderers.
	void end();

};

//------------------------------------------------------------------------------
// SPRITE BATCH - storage structure for saving a reuseable
// command-buffer of draw calls (decoupled from plotter so that
// calls can be freely mixed with other systems).
//------------------------------------------------------------------------------

// An efficient system for storing sprites and rendering sprites in a batch.
// Because the batch implements a deque, sprites can be on one of two layers 
// (top or bottom), but are not well z-ordered within a layer.
//
// ?? generic frame-based "animators" ??
// ?? >2 multilayer ??

//TODO: typedef uint32_t SpriteID;

// Create a new sprite context from a given capacity.
SpriteBatch *createSpriteBatch(size_t capacity=1024);

class SpriteBatchRef {
private:
	SpriteBatch *context;

public:
	SpriteBatchRef() {}
	SpriteBatchRef(SpriteBatch *aContext) : context(aContext) {}

	operator SpriteBatch*() { return context; }
	operator bool() const { return context; }

	void destroy();

	Sprite* addSprite(
		ImageAsset *image, 
		const AffineMatrix *xform,
		int frame=0, 
		Color c=rgba(0), 
		bool visible=1, 
		bool onTop=0, 
		void *userData=0
	);

	void draw(SpritePlotterRef plotter);

};

//------------------------------------------------------------------------------
// BATCHED SPRITE INSTANCE
//------------------------------------------------------------------------------

class SpriteRef {
private:
	Sprite* sprite;

public:
	SpriteRef() {}
	SpriteRef(Sprite *aSprite) : sprite(aSprite) {}

	operator Sprite*() { return sprite; }
	operator bool() const { return sprite; }

	void destroy();

	void setLayer(int layer);
	void setImage(ImageAsset *img);
	void setTransform(const AffineMatrix* matrix);
	void setFrame(int frame);
	void setVisible(bool flag);
	void setColor(Color c);
	void setUserData(void *userData);
	
	int layer() const;
	const AffineMatrix* transform() const;
	ImageAsset *image() const;
	bool visible() const;
	int frame() const;
	Color color() const;
	void *userData() const;

	template<typename T>
	T* get() const { return (T*) userData(); }
};



