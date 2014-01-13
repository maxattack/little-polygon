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
#include "littlepolygon_templates.h"


struct SpriteDraw {
	ImageAsset *image;
	AffineMatrix xform;
	Color color;
	uint32_t slot : 10;
	uint32_t frame : 8;
	uint32_t visible : 1;
	uint32_t unused : 13;

	bool isVisible() const { return image && visible; }
};

struct Sprite {
	SpriteDraw *cmd;
	void *userData;
	uint32_t layer;
};

struct SpriteBatch {
	size_t capacity;
	size_t bottomCount;
	size_t topCount;
	Bitset<1024> allocationMask;

	Sprite headSprite;

	bool owns(Sprite* sprite) const {
		auto offset = sprite - &headSprite;
		return 0 <= offset && offset < capacity;
	}

	size_t count() const { 
		return bottomCount + topCount; 
	}

	Sprite *get(int index) {
		ASSERT(allocationMask[index]);
		return &headSprite + index;
	}

	SpriteDraw *getDraw(int index) {
		SpriteDraw *headDraw = (SpriteDraw*) (&headSprite + capacity);
		return headDraw + index;
	}
};

SpriteBatch *createSpriteBatch(size_t capacity) {
	// validate args
	ASSERT(capacity <= 1024);

	// alloc memory
	auto context = (SpriteBatch*) LITTLE_POLYGON_MALLOC(
		sizeof(SpriteBatch) + 
		sizeof(Sprite) * (capacity - 1) + 
		sizeof(SpriteDraw) * (capacity)
	);

	// init fields
	context->capacity = capacity;
	context->bottomCount = 0;
	context->topCount =0 ;
	context->allocationMask = Bitset<1024>();
	return context;
}

void destroy(SpriteBatch *context) {
	LITTLE_POLYGON_FREE(context);
}

static void addToLayer(SpriteBatch *context, Sprite *sprite, int layer) {
	if (layer == 0) {
		sprite->cmd = context->getDraw(context->bottomCount);
		++context->bottomCount;
	} else {
		sprite->cmd = context->getDraw(context->capacity - context->topCount);
		++context->topCount;
	}
	sprite->layer = layer;
}

static void removeFromLayer(SpriteBatch *context, Sprite *sprite) {
	if (sprite->layer == 0) {
		// remove from bottom of the layer
		int index = sprite->cmd - context->getDraw(0);
		if (index != context->bottomCount-1) {
			// swap with the last element
			auto drawToMove = context->getDraw(context->bottomCount-1);
			auto spriteToMove = context->get(drawToMove->slot);
			spriteToMove->cmd = sprite->cmd;
			*(spriteToMove->cmd) = *drawToMove;
		}
		--context->bottomCount;
	} else {
		// remove from top of the layer
		int index = context->getDraw(context->capacity) - sprite->cmd;
		if (index != context->topCount-1) {
			// swap with the first element
			auto drawToMove = context->getDraw(context->capacity - (context->topCount-1));
			auto spriteToMove = context->get(drawToMove->slot);
			spriteToMove->cmd = sprite->cmd;
			*(spriteToMove->cmd) = *drawToMove;
		}
		--context->topCount;
	}
}

Sprite* createSprite(
	SpriteBatch *context, 
	const AffineMatrix& xform,
	ImageAsset *image, int frame, Color c, bool visible, bool onTop, 
	void *userData
) {
	ASSERT(context->count() < context->capacity);

	unsigned slot;
	if (!(~context->allocationMask).clearFirst(slot)) {
		return 0;
	}

	// allocate
	context->allocationMask.mark(slot);
	auto result = context->get(slot);
	
	// setup layer
	addToLayer(context, result, !!onTop);

	// setup draw call
	result->cmd->slot = slot;
	result->cmd->xform = xform;
	result->cmd->color = c;
	result->cmd->image = image;
	result->cmd->frame = frame;
	result->cmd->visible = visible;
	
	result->userData = userData;

	return result;
}

void destroy(SpriteBatch *context, Sprite* sprite) {
	ASSERT(context->owns(sprite));
	removeFromLayer(context, sprite);
	context->allocationMask.clear(sprite - &context->headSprite);
}

void setLayer(SpriteBatch *context, Sprite* sprite, int layerIdx) {
	ASSERT(context->owns(sprite));
	if (sprite->layer != layerIdx) {
		auto cmd = *sprite->cmd;
		removeFromLayer(context, sprite);
		addToLayer(context, sprite, layerIdx);
		*sprite->cmd = cmd;
	}
}

void setTransform(Sprite *sprite, const AffineMatrix &matrix) {
	sprite->cmd->xform = matrix;
}

void setPosition(Sprite *sprite, vec2 p) {
	sprite->cmd->xform.t = p;
}

void setFlipped(Sprite *sprite, bool flipped) {
	sprite->cmd->xform.u = vec(flipped ? -1 : 1, 0);
}

void setRotation(Sprite *sprite, float radians) {
	float s = sinf(radians);
	float c = cosf(radians);
	sprite->cmd->xform.u = vec(c,s);
	sprite->cmd->xform.v = vec(-s,c);
}

void setScale(Sprite *sprite, float scale) {
	sprite->cmd->xform.u = vec(scale, 0);
	sprite->cmd->xform.v = vec(0, scale);
}


void setImage(Sprite* sprite, ImageAsset *image) {
	sprite->cmd->image = image;
}

void setFrame(Sprite* sprite, int frame) {
	sprite->cmd->frame = frame;
}

void setVisible(Sprite* sprite, bool visible) {
	sprite->cmd->visible = visible;
}

void setColor(Sprite* sprite, Color c) {
	sprite->cmd->color = c;
}

void setUserData(Sprite* sprite, void *userData) {
	sprite->userData = userData;
}

ImageAsset *image(Sprite* sprite) {
	return sprite->cmd->image;
}

AffineMatrix transform(Sprite *sprite) {
	return sprite->cmd->xform;
}

int frame(Sprite* sprite) {
	return sprite->cmd->frame;
}

int layer(Sprite* sprite) {
	return sprite->layer;
}

bool visible(Sprite* sprite) {
	return sprite->cmd->visible;
}

Color color(Sprite* sprite) {
	return sprite->cmd->color;
}

void *userData(Sprite* sprite) {
	return sprite->userData;
}

static void draw(SpriteDraw *cmd, SpritePlotter *renderer) {
	if (cmd->isVisible()) {
		drawImageTransformed(renderer, cmd->image, cmd->xform, cmd->frame, cmd->color);
	}	
}

void draw(SpriteBatch *context, SpritePlotter *renderer) {
	for(int i=0; i<context->bottomCount; ++i) {
		draw(context->getDraw(i), renderer);
	}
	for(int i=0; i<context->topCount; ++i) {
		draw(context->getDraw(context->capacity - (context->topCount-1) + i), renderer);
	}

}
