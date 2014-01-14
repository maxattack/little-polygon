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
#include "littlepolygon_utils.h"

//------------------------------------------------------------------------------
// SPRITE PLOTTER
//------------------------------------------------------------------------------

// This object can render lots of sprites in a small number of batched draw calls
// by coalescing adjacent draws into larger logical draws.
// TODO: perform batch-level clipping?
struct SpritePlotter;
SpritePlotter *createSpritePlotter(int capacity=64);
void destroy(SpritePlotter *context);

// Call this method to initialize the graphics context state.  Coordinates are
// set to a orthogonal projection matrix, and some basic settings like blending are
// enabled.  Any additional state changes can be set *after* this function but *before*
// issuing any draw calls.
void begin(SpritePlotter* context, vec2 canvasSize, vec2 scrolling=vec(0,0));

// Draw the given image.  Will potentially cause a draw call to actually be emitted
// to the graphics device if: (i) the buffer has reached capacity or (ii) the texture 
// atlas has changed.  Color transforms *can* be batched, because they are encoded
// in the vertices, not in shader uniforms.
void drawImage(SpritePlotter* context, ImageAsset *image, vec2 position, int frame=0, Color color=rgba(0));
void drawImageTransformed(SpritePlotter* context, ImageAsset *image, vec2 position, vec2 attitude, int frame=0, Color color=rgba(0));
void drawImageTransformed(SpritePlotter *context, ImageAsset *image, const AffineMatrix& xform, int frame=0, Color color=rgba(0));
void drawImageTransformed(SpritePlotter *context, ImageAsset *image, const mat4f& xform, int frame=0, Color color=rgba(0));
void drawImageRotated(SpritePlotter* context, ImageAsset *image, vec2 position, float radians, int frame=0, Color color=rgba(0));
void drawImageScaled(SpritePlotter* context, ImageAsset *image, vec2 position, vec2 k, int frame=0, Color color=rgba(0));
void drawLabel(SpritePlotter* context, FontAsset *font, vec2 p, Color c, const char *msg);
void drawLabelCentered(SpritePlotter* context, FontAsset *font, vec2 p, Color c, const char *msg);
void drawTilemap(SpritePlotter* context, TilemapAsset *map, vec2 position=vec(0,0));

// if you want to monkey with the global rendering state (e.g. change blending settings)
// you need to flush the render queue first.
void flush(SpritePlotter* context);

// Commit the current draw queue and return the graphics context state to it's
// canonical form, to play nice with other renderers.
void end(SpritePlotter* context);

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

struct SpriteBatch;
struct Sprite;

// Create a new sprite context from a given capacity.
SpriteBatch *createSpriteBatch(size_t capacity=1024);
void destroy(SpriteBatch *context);

// Create a new sprite
Sprite* createSprite(
	SpriteBatch *context, 
	const AffineMatrix& xform,
	ImageAsset *image, 
	int frame=0, 
	Color c=rgba(0), 
	bool visible=1, 
	bool onTop=0, 
	void *userData=0
);
void destroy(SpriteBatch *context, Sprite* sprite);

// setters
void setLayer(SpriteBatch *context, Sprite* sprite, int layer);

void setTransform(Sprite *sprite, const AffineMatrix &matrix);

// convenience transform methods (will squash existing values)
void setPosition(Sprite *sprite, vec2 p);
void setFlipped(Sprite *sprite, bool flipped);
void setRotation(Sprite *sprite, float radians);
void setScale(Sprite *sprite, float scale);

void setImage(Sprite* sprite, ImageAsset *image);
void setFrame(Sprite* sprite, int frame);
void setVisible(SpriteBatch *context, Sprite* sprite, bool visible);
void setColor(Sprite* sprite, Color c);
void setUserData(Sprite* sprite, void *userData);

// getters
int layer(Sprite *sprite);
AffineMatrix transform(Sprite *sprite);
ImageAsset *image(Sprite* sprite);
int frame(Sprite* sprite);
bool visible(SpriteBatch *context, Sprite* sprite);
Color color(Sprite* sprite);
void *userData(Sprite* sprite);

// batch methods
// void advanceAnimations(SpriteBatch *context, float dt);
void draw(Sprite *sprite, SpritePlotter *plotter);
void draw(SpriteBatch *context, SpritePlotter *plotter);