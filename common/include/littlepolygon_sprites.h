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

struct SpriteBatch;
typedef uint32_t SPRITE;

// Create a new sprite context from a given capacity.
SpriteBatch *createSpriteBatch(size_t numLayers=8, size_t layerCapacity=128, size_t spriteCapacity=1024);
void destroy(SpriteBatch *context);

// Create a new sprite
SPRITE createSprite(SpriteBatch *context, ImageAsset *image, int layer=0, SPRITE explicitId=0);
void destroy(SpriteBatch *context, SPRITE sprite);

// setters
void setImage(SpriteBatch *context, SPRITE sprite, ImageAsset *image);
void setFrame(SpriteBatch *context, SPRITE sprite, int frame);
void setLayer(SpriteBatch *context, SPRITE sprite, int layer);
void setVisible(SpriteBatch *context, SPRITE sprite, bool visible);
void setColor(SpriteBatch *context, SPRITE sprite, Color c);
void setUserData(SpriteBatch *context, SPRITE sprite, void *userData);

// getters
ImageAsset *image(SpriteBatch *context, SPRITE sprite);
int frame(SpriteBatch *context, SPRITE sprite);
int layer(SpriteBatch *context, SPRITE sprite);
bool visible(SpriteBatch *context, SPRITE sprite);
Color color(SpriteBatch *context, SPRITE sprite);
void *userData(SpriteBatch *context, SPRITE sprite);

// batch methods
// void advanceAnimations(SpriteBatch *context, float dt);
void drawLayer(SpriteBatch *context, int layerIdx, SpritePlotter *plotter);
void draw(SpriteBatch *context, SpritePlotter *plotter);
