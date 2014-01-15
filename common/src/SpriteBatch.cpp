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
	const AffineMatrix *xform;
	Color color;
	uint16_t slot;
	uint16_t frame;
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
	Bitset<1024> visibleMask;

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

	void markVisible(SpriteDraw *draw) {
		SpriteDraw *headDraw = (SpriteDraw*) (&headSprite + capacity);
		visibleMask.mark(draw - headDraw);
	}

	void clearVisible(SpriteDraw *draw) {
		SpriteDraw *headDraw = (SpriteDraw*) (&headSprite + capacity);
		visibleMask.clear(draw - headDraw);
	}

	bool visible(SpriteDraw *draw) const {
		SpriteDraw *headDraw = (SpriteDraw*) (&headSprite + capacity);
		return visibleMask[ draw - headDraw ];
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
	context->visibleMask = Bitset<1024>();
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
	context->clearVisible(sprite->cmd);
	if (sprite->layer == 0) {
		// remove from bottom of the layer
		int index = sprite->cmd - context->getDraw(0);
		if (index != context->bottomCount-1) {
			// swap with the last element
			auto drawToMove = context->getDraw(context->bottomCount-1);
			auto wasVisible = context->visible(drawToMove);
			context->clearVisible(drawToMove);
			auto spriteToMove = context->get(drawToMove->slot);
			spriteToMove->cmd = sprite->cmd;
			*(spriteToMove->cmd) = *drawToMove;
			if (wasVisible) {
				context->markVisible(spriteToMove->cmd);
			}
		}
		--context->bottomCount;
	} else {
		// remove from top of the layer
		int index = context->getDraw(context->capacity) - sprite->cmd;
		if (index != context->topCount-1) {
			// swap with the first element
			auto drawToMove = context->getDraw(context->capacity - (context->topCount-1));
			auto wasVisible = context->visible(drawToMove);
			context->clearVisible(drawToMove);
			auto spriteToMove = context->get(drawToMove->slot);
			spriteToMove->cmd = sprite->cmd;
			*(spriteToMove->cmd) = *drawToMove;
			if (wasVisible) {
				context->markVisible(spriteToMove->cmd);
			}			
		}
		--context->topCount;
	}
}

Sprite* createSprite(
	SpriteBatch *context, 
	ImageAsset *image, 
	const AffineMatrix *xform,
	int frame, Color c, bool visible, bool onTop, 
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
	if (visible) {
		context->markVisible(result->cmd);
	}
	
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
		bool wasVisible = context->visible(sprite->cmd);
		context->clearVisible(sprite->cmd);

		auto cmd = *sprite->cmd;
		removeFromLayer(context, sprite);
		addToLayer(context, sprite, layerIdx);
		
		*sprite->cmd = cmd;
		if (wasVisible) {
			context->markVisible(sprite->cmd);
		}
	}
}

void setImage(Sprite* sprite, ImageAsset *image) {
	sprite->cmd->image = image;
}

void setTransform(Sprite *sprite, const AffineMatrix *xform) {
	sprite->cmd->xform = xform;
}

void setFrame(Sprite* sprite, int frame) {
	sprite->cmd->frame = frame;
}

void setVisible(SpriteBatch *context, Sprite* sprite, bool visible) {
	if (visible) {
		context->markVisible(sprite->cmd);
	} else {
		context->clearVisible(sprite->cmd);
	}
}

void setColor(Sprite* sprite, Color c) {
	sprite->cmd->color = c;
}

void setUserData(Sprite* sprite, void *userData) {
	sprite->userData = userData;
}

ImageAsset *image(const Sprite* sprite) {
	return sprite->cmd->image;
}

const AffineMatrix *transform(const Sprite *sprite) {
	return sprite->cmd->xform;
}

int frame(const Sprite* sprite) {
	return sprite->cmd->frame;
}

int layer(const Sprite* sprite) {
	return sprite->layer;
}

bool visible(SpriteBatch *context, Sprite* sprite) {
	return context->visible(sprite->cmd);
}

Color color(const Sprite* sprite) {
	return sprite->cmd->color;
}

void *userData(const Sprite* sprite) {
	return sprite->userData;
}

void draw(SpriteDraw *cmd, SpritePlotter *renderer) {
	drawImageTransformed(renderer, cmd->image, *cmd->xform, cmd->frame, cmd->color);
}

void draw(SpriteBatch *context, SpritePlotter *renderer) {
	unsigned idx;
	for(auto iterator=context->visibleMask.listBits(); iterator.next(idx);) {
		draw(context->getDraw(idx), renderer);
	}
}
