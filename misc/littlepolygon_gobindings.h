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

#include "littlepolygon_go.h"
#include "littlepolygon_fk.h"
#include "littlepolygon_sprites.h"

// Binds various systems to the GO database, and declares assets for them
// to be loaded from an asset bundle.
// (and, gift idea, binds to lua... or squirrel...?)

// VERY WIP !!

#define COMPONENT_TYPE_NODE 0
#define COMPONENT_TYPE_SPRITE 1

class GameObjectRef {
private:
	GoContext *context;
	GO go;

	GameObjectRef(); // no default ctor

public:
	GameObjectRef(GoContext *aContext, GO aGo) :
		context(aContext), go(aGo) {}

	static GameObjectRef find(GoContext *context, const char *name) {
		return GameObjectRef(context, ::find(context, name));
	}

	operator GO() const { return go; }

	const char* name() { return goName(context, go); }
	bool enabled() { return goEnabled(context, go); }

	void enable() { ::enable(context, go); }
	void disable() { ::disable(context, go); }
};

//------------------------------------------------------------------------------
// NODE COMPONENT
// Endows GOs with a position in space and a heirarchical structure.
//------------------------------------------------------------------------------

struct NodeAsset {
	// parent?
	float matrix[16];
};

// GO methods
GoComponentDef nodeDef(FkContext *context);

void addNode(GoContext *context, GO go, const NodeAsset *asset=0) {
	addComponent(context, go, COMPONENT_TYPE_NODE, asset);
}

inline NodeRef getNode(GoContext *context, GO go) { 
	return NodeRef(
		(FkContext*) getContext(context, COMPONENT_TYPE_NODE),
		(Node*) getComponent(context, go, COMPONENT_TYPE_NODE)->userData
	);
}

//------------------------------------------------------------------------------
// Sprite *COMPONENT
// Maintains a sprite batch and a draw queue that renders sprites with the 
// node's transformation.
//------------------------------------------------------------------------------

class SpriteRef {
private:
	SpriteBatch *context;
	Sprite *sprite;

public:
	SpriteRef(SpriteBatch *aContext, Sprite *aSprite) : 
		context(aContext), sprite(aSprite) {}

	GoComponent *component() { return (GoComponent*) userData(sprite); }
	ImageAsset *image() { return ::image(sprite); }
	int frame() { return ::frame(sprite); }
	int layer() { return ::layer(sprite); }
	bool visible() { return ::visible(sprite); }
	Color color() { return ::color(sprite); }

	void setLayer(int layer) { ::setLayer(context, sprite, layer); }
	void setImage(ImageAsset *image) { ::setImage(sprite, image); }
	void setFrame(int frame) { ::setFrame(sprite, frame); }
	void setVisible(bool visible) { ::setVisible(sprite, visible); }
	void setColor(Color c) { ::setColor(sprite, c); }

};

struct SpriteAsset {
	uint32_t imageHash;
	vec2 position;
	Color color;
};

GoComponentDef spriteDef(SpriteBatch *context, AssetBundle *assets);

void addSprite(GoContext *context, GO go, const SpriteAsset *asset=0) {
	addComponent(context, go, COMPONENT_TYPE_SPRITE, asset);
}

inline SpriteRef getSprite(GoContext *context, GO go) {
	return SpriteRef(
		(SpriteBatch*) getContext(context, COMPONENT_TYPE_SPRITE),
		(Sprite*) getComponent(context, go, COMPONENT_TYPE_SPRITE)->userData
	);
}

