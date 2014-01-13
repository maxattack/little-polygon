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

#include "littlepolygon_graphics.h"

// An efficient system for storing sprites in Z-sorted "layers" and clipping/rendering
// in a batch instead of emitting lots of individual draw-calls.  Useable on
// it's own or in conjunction with the fk display tree via the go module.
//
// TODO: generic frame-based "animators" ?

struct SpriteContext;
typedef uint32_t SPRITE;

// Create a new sprite context from a given capacity.
SpriteContext *createSpriteContext(size_t numLayers=8, size_t layerCapacity=128, size_t spriteCapacity=1024);
void destroy(SpriteContext *context);

// Create a new sprite
SPRITE createSprite(SpriteContext *context, ImageAsset *image, int layer=0, SPRITE explicitId=0);
void destroy(SpriteContext *context, SPRITE sprite);

// setters
void setImage(SpriteContext *context, SPRITE sprite, ImageAsset *image);
void setFrame(SpriteContext *context, SPRITE sprite, int frame);
void setLayer(SpriteContext *context, SPRITE sprite, int layer);
void setVisible(SpriteContext *context, SPRITE sprite, bool visible);
void setColor(SpriteContext *context, SPRITE sprite, Color c);
void setUserData(SpriteContext *context, SPRITE sprite, void *userData);

// getters
ImageAsset *image(SpriteContext *context, SPRITE sprite);
int frame(SpriteContext *context, SPRITE sprite);
int layer(SpriteContext *context, SPRITE sprite);
bool visible(SpriteContext *context, SPRITE sprite);
Color color(SpriteContext *context, SPRITE sprite);
void *userData(SpriteContext *context, SPRITE sprite);

// batch methods
// void advanceAnimations(SpriteContext *context, float dt);
void drawLayer(SpriteContext *context, int layerIdx, SpritePlotter *r);
void draw(SpriteContext *context, SpritePlotter *r);
