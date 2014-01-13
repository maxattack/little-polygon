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
void setImage(Sprite* sprite, ImageAsset *image);
void setFrame(Sprite* sprite, int frame);
void setVisible(Sprite* sprite, bool visible);
void setColor(Sprite* sprite, Color c);
void setUserData(Sprite* sprite, void *userData);

// getters
int layer(Sprite *sprite);
ImageAsset *image(Sprite* sprite);
int frame(Sprite* sprite);
bool visible(Sprite* sprite);
Color color(Sprite* sprite);
void *userData(Sprite* sprite);

// batch methods
// void advanceAnimations(SpriteBatch *context, float dt);
void draw(SpriteBatch *context, SpritePlotter *plotter);
