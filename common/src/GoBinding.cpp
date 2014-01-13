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

#include "littlepolygon_gobindings.h"

//------------------------------------------------------------------------------
// NODES
//------------------------------------------------------------------------------

static NODE firstChild(FkContext *context, NODE node) {
	return FkChildIterator(context, node).current;
}

static int nodeInit(GoContext *context, GoComponent *component, const void *args, void *user) {	
	// ?? should parent-child attachments happen *after* init?  It would decouple
	//    potential order-of-initialization issues...

	// type-cast args
	auto fkContext = (FkContext*) user;

	// create backing store
	NODE result;
	if (args) {
		result = createNode(fkContext, 0, component, 0);
	} else {
		auto nodeArgs = (NodeAsset*) args;
		result = createNode(fkContext, nodeArgs->parent, component, nodeArgs->id);

		// initialize geom
		mat4f m;
		m.load(nodeArgs->matrix);
		setLocal(fkContext, result, m);
	}

	// save reference to metadata
	component->userData = (void*) result;

	// everything OK
	return 0;
}

static int nodeDestroy(GoContext *context, GoComponent *component, const void *args, void *user) {	
	

	// typecase args
	auto fkContext = (FkContext*) user;
	auto node = (NODE) component->userData;

	// recursively destroy children
	for (NODE child = firstChild(fkContext, node); child; child=firstChild(fkContext, node)) {
		// ?? will calling destroy within a message like this mess up the GO system ??
		destroy(
			context, 
			((GoComponent*)userData(fkContext, child))->go
		);
	}

	// now destroy this node
	destroy(fkContext, node);

	// everything OK
	return 0;
}

GoComponentDef nodeDef(FkContext *context) {
	GoComponentDef result = { context, nodeInit, 0, 0, nodeDestroy };
	return result;
}

//------------------------------------------------------------------------------
// SPRITES
//------------------------------------------------------------------------------

struct Sprite {
	union {
		struct {
			// active params
			GoComponent *component;
			int layer;
			int index;
		};
		struct {
			// inactive params
			Sprite *next;
			Sprite *prev;
		};
	};
};

struct SpriteDraw {
	ImageAsset *image;
	vec2 position;
	Color color;
	uint32_t slot : 16;
	uint32_t frame : 14;
	uint32_t visible : 1;
	uint32_t enabled : 1;

	bool isVisible() const { return visible && enabled; }
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
	AssetBundle *assets;
	Sprite *firstFree;

	Sprite headSprite;

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
};

SpriteContext *createSpriteContext(AssetBundle *assets, size_t numLayers, size_t layerCapacity) {
	// validate args
	ASSERT(numLayers < 32); // seems reasonable
	ASSERT(layerCapacity < 1024);
	ASSERT(layerCapacity > numLayers);

	auto spriteCap = numLayers * layerCapacity;
	ASSERT(spriteCap < 0x10000);

	// alloc memory
	auto context = (SpriteContext*) LITTLE_POLYGON_MALLOC(
		sizeof(SpriteContext) + 
		sizeof(Sprite) * (spriteCap - 1) + 
		sizeof(SpriteLayer) * layerCapacity + 
		sizeof(SpriteDraw) * (layerCapacity-1) * numLayers
	);

	// init fields
	context->spriteCapacity = spriteCap;
	context->spriteCount = 0;
	context->numLayers = numLayers;
	context->layerCapacity = layerCapacity;
	context->assets = assets;
	
	// init sprite freelist
	auto sbuf = context->getSprite(0);
	context->firstFree = sbuf;
	for(int i=0; i<spriteCap; ++i) {
		sbuf[i].next = i == spriteCap-1 ? 0 : &sbuf[i+1];
		sbuf[i].prev = i == 0 ? 0 : &sbuf[i-1];
		
	}

	// init layer counts
	for(int i=0; i<numLayers; ++i) {
		context->getLayer(i)->count = 0;
	}

	return context;
}

void destroy(SpriteContext *context) {
	LITTLE_POLYGON_FREE(context);
}

void draw(SpriteContext *context, SpriteBatch *batch) {
	for(int i=0; i<context->numLayers; ++i) {
		auto layer = context->getLayer(i);
		for(int j=0; j<layer->count; ++j) {
			auto drawCall = layer->get(j);
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
}

static int spriteInit(GoContext *goContext, GoComponent *component, const void *args, void *user) {	
	// cast args
	auto context = (SpriteContext*) user;
	auto asset = (const SpriteAsset*) args;
	
	// pop a sprite from the freelist
	ASSERT(context->firstFree);
	auto result = context->firstFree;
	context->firstFree = result->next;
	if (context->firstFree) { context->firstFree->prev = 0; }

	// bidirectional mapping
	result->component = component;
	component->userData = result;

	// add to layer
	result->layer = asset ? asset->layer : 0;
	auto layer = context->getLayer(result->layer);
	result->index = layer->count;
	layer->count++;

	// setup draw call
	auto drawCall = layer->get(result->index);
	drawCall->slot = int(result - context->getSprite(0));
	drawCall->position = asset ? vec(asset->x, asset->y) : vec(0,0);
	drawCall->color = asset ? asset->color : rgba(0x00000000);
	drawCall->image = asset ? context->assets->image(asset->imageHash) : 0;
	drawCall->frame = asset ? asset->frame : 0;
	drawCall->visible = asset ? asset->visible : 1;
	drawCall->enabled = goEnabled(goContext, component->go);

	// all good
	return 0;
}

static int spriteEnable(GoContext *goContext, GoComponent *component, const void *args, void *user) {	
	auto context = (SpriteContext*) user;
	auto sprite = (Sprite*) component->userData;
	context->getSpriteDraw(sprite)->enabled = 1;
	return 0;
}

static int spriteDisable(GoContext *goContext, GoComponent *component, const void *args, void *user) {	
	auto context = (SpriteContext*) user;
	auto sprite = (Sprite*) component->userData;
	context->getSpriteDraw(sprite)->enabled = 0;
	return 0;
}

static int spriteDestroy(GoContext *goContext, GoComponent *component, const void *args, void *user) {	
	auto context = (SpriteContext*) user;
	auto sprite = (Sprite*) component->userData;

	// remove from layer (swap with last element)
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

	// return to free list
	sprite->next = context->firstFree;
	sprite->prev = 0;
	if (context->firstFree) { context->firstFree->prev = sprite; }
	context->firstFree = sprite;

	return 0;
}

GoComponentDef spriteDef(SpriteContext *context) {
	GoComponentDef result = { context, spriteInit, spriteEnable, spriteDisable, spriteDestroy };
	return result;
}



