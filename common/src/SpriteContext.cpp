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

#define SPRITE_INDEX(id) ((0xffff&id)-1)

struct Sprite {
	SPRITE handle;
	int layer;
	int index;
	void *userData;
};

struct SpriteDraw {
	ImageAsset *image;
	vec2 position;
	Color color;
	uint32_t slot : 10;
	uint32_t frame : 8;
	uint32_t visible : 1;
	uint32_t unused : 13;

	bool isVisible() const { return image && visible; }
};

struct SpriteLayer {
	size_t count;
	SpriteDraw headDraw;

	SpriteDraw *get(int i) { 
		ASSERT(i < count);
		return (&headDraw) + i;
	}	
};

struct SpriteContext {
	size_t spriteCapacity;
	size_t spriteCount;
	size_t numLayers;
	size_t layerCapacity;
	Bitset<1024> allocationMask;

	Sprite headSprite;

	Sprite *lookup(SPRITE s) {
		auto i = SPRITE_INDEX(s);
		ASSERT(allocationMask[i]);
		auto result = getSprite(i);
		ASSERT(result->handle == s);
		return result;
	}

	Sprite *getSprite(int i) {
		ASSERT(i < spriteCapacity);
		return (&headSprite) + i;
	}

	SpriteLayer *getLayer(int i) {
		ASSERT(i < numLayers);
		uint8_t *first = (uint8_t*) getSprite(spriteCapacity);
		size_t bytesPerSpriteLayer = sizeof(SpriteLayer) + sizeof(SpriteDraw) * (layerCapacity-1);
		return (SpriteLayer*) (first + bytesPerSpriteLayer);
	};

	SpriteDraw *getSpriteDraw(Sprite *s) {
		return getLayer(s->layer)->get(s->index);
	}

	SpriteDraw *lookupDraw(SPRITE s) {
		return getSpriteDraw(lookup(s));
	}
};

SpriteContext *createSpriteContext(AssetBundle *assets, size_t numLayers, size_t layerCapacity, size_t spriteCapacity) {
	// validate args
	ASSERT(numLayers <= 32); // seems reasonable
	ASSERT(layerCapacity <= 1024);
	ASSERT(spriteCapacity <= 1024);

	// alloc memory
	auto context = (SpriteContext*) LITTLE_POLYGON_MALLOC(
		sizeof(SpriteContext) + 
		sizeof(Sprite) * (spriteCapacity - 1) + 
		sizeof(SpriteLayer) * layerCapacity + 
		sizeof(SpriteDraw) * (layerCapacity-1) * numLayers
	);

	// init fields
	context->spriteCapacity = spriteCapacity;
	context->spriteCount = 0;
	context->numLayers = numLayers;
	context->layerCapacity = layerCapacity;
	context->allocationMask = Bitset<1024>();
	
	// init layer counts
	for(int i=0; i<numLayers; ++i) {
		context->getLayer(i)->count = 0;
	}

	// init sprite handles
	for(int i=0; i<spriteCapacity; ++i) {
		context->getSprite(i)->handle = i+1;
	}

	return context;
}

void destroy(SpriteContext *context) {
	LITTLE_POLYGON_FREE(context);
}

void drawLayer(SpriteContext *context, int layerIdx, SpriteBatch *batch) {
	auto layer = context->getLayer(layerIdx);
	for(int j=0; j<layer->count; ++j) {
		auto drawCall = layer->get(j);
		// todo: clipping?
		if (drawCall->isVisible()) {
			drawImage(
				batch,
				drawCall->image, 
				drawCall->position, 
				drawCall->frame, 
				drawCall->color
			);
		}
	}
}

void draw(SpriteContext *context, SpriteBatch *batch) {
	for(int i=0; i<context->numLayers; ++i) {
		drawLayer(context, i, batch);
	}
}

SPRITE createSprite(SpriteContext *context, ImageAsset *image, int layerIdx, SPRITE explicitId) {
	ASSERT(context->spriteCount < context->spriteCapacity);
	ASSERT(layerIdx < context->numLayers);

	unsigned index;
	if (explicitId) {
		index = SPRITE_INDEX(explicitId);
		ASSERT(index < context->spriteCapacity);
		if(context->allocationMask[index]) {
			return 0;
		}
	} else {
		if (!(~context->allocationMask).clearFirst(index)) {
			return 0;
		}
	}

	// allocate
	context->allocationMask.mark(index);
	auto result = context->getSprite(index);
	
	// setup layer
	result->layer = layerIdx;
	auto layer = context->getLayer(layerIdx);
	ASSERT(layer->count < context->layerCapacity);
	result->index = layer->count;
	layer->count++;

	// setup draw call
	auto drawCall = layer->get(result->index);
	drawCall->slot = int(result - context->getSprite(0));
	drawCall->position = vec(0,0);
	drawCall->color = rgba(0x00000000);
	drawCall->image = image;
	drawCall->frame = 0;
	drawCall->visible = 1;
	// drawCall->enabled = 1;

	return result->handle;
}

static void removeFromLayer(SpriteContext *context, Sprite *sprite) {
	auto layer = context->getLayer(sprite->layer);
	ASSERT(layer->count > 0);
	if (context->getSpriteDraw(sprite) != layer->get(layer->count-1)) {
		auto relocated = context->getSprite(
			layer->get(layer->count-1)->slot
		);
		relocated->index = sprite->index;
		*layer->get(sprite->index) = *layer->get(layer->count-1);
	}
	--layer->count;	
}

void destroy(SpriteContext *context, SPRITE hSprite) {
	auto sprite = context->lookup(hSprite);
	removeFromLayer(context, sprite);

	// dealloc
	context->allocationMask.clear(SPRITE_INDEX(hSprite));
}

void setImage(SpriteContext *context, SPRITE sprite, ImageAsset *image) {
	context->lookupDraw(sprite)->image = image;
}

void setFrame(SpriteContext *context, SPRITE sprite, int frame) {
	context->lookupDraw(sprite)->frame = frame;
}

void setLayer(SpriteContext *context, SPRITE sprite, int layerIdx) {
	ASSERT(layerIdx < context->numLayers);
	auto s = context->lookup(sprite);
	
	// make sure we actually *need* to change
	if (s->layer != layerIdx) {

		// save the draw call
		auto cmd = *context->lookupDraw(sprite);

		// remove from current layer
		removeFromLayer(context, s);

		// register with new layer
		auto layer = context->getLayer(layerIdx);
		ASSERT(layer->count < context->layerCapacity);
		s->layer = layerIdx;
		s->index = layer->count;
		layer->count++;

		// write the draw call
		*layer->get(s->index) = cmd;

	}
}

void setVisible(SpriteContext *context, SPRITE sprite, bool visible) {
	context->lookupDraw(sprite)->visible = visible;
}

void setColor(SpriteContext *context, SPRITE sprite, Color c) {
	context->lookupDraw(sprite)->color = c;
}

void setUserData(SpriteContext *context, SPRITE sprite, void *userData) {
	context->lookup(sprite)->userData = userData;
}

ImageAsset *image(SpriteContext *context, SPRITE sprite) {
	return context->lookupDraw(sprite)->image;
}

int frame(SpriteContext *context, SPRITE sprite) {
	return context->lookupDraw(sprite)->frame;
}

int layer(SpriteContext *context, SPRITE sprite) {
	return context->lookup(sprite)->layer;
}

bool visible(SpriteContext *context, SPRITE sprite) {
	return context->lookupDraw(sprite)->visible;
}

Color color(SpriteContext *context, SPRITE sprite) {
	return context->lookupDraw(sprite)->color;
}

void *userData(SpriteContext *context, SPRITE sprite) {
	return context->lookup(sprite)->userData;
}

