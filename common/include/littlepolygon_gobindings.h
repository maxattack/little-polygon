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
#include "littlepolygon_graphics.h"

// Binds various systems to the GO database, and declares assets for them
// to be loaded from an asset bundle.
// (and, gift idea, binds to lua... or squirrel...?)

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
// ? Option to use contexts other than the FkContext 
//   (e.g. Simple2D, IK, parametric, etc)
//------------------------------------------------------------------------------

class NodeRef {
private:
	FkContext *context;
	NODE node;

	NodeRef(); // no default ctor

public:
	NodeRef(FkContext *aContext, NODE aNode) : 
		context(aContext), node(aNode) {}

	operator NODE() { return node; }

	GoComponent *component() { return (GoComponent*) userData(context, node); }
	NodeRef parent() { return NodeRef(context, ::parent(context, node)); }
	mat4f localToParent() { return local(context, node); }
	mat4f localToWorld() { return world(context, node); }

	void addChild(NodeRef child) { setParent(context, child.node, node); }
	void reparentTo(NodeRef parent) { reparent(context, node, parent.node); }
	void unparent() { setParent(context, node, 0); }
	void detachChildren() { ::detachChildren(context, node); }

	void setLocal(mat4f tform) { ::setLocal(context, node, tform); }
	void setWorld(mat4f tform) { ::setWorld(context, node, tform); }
};

struct NodeAsset {
	NODE id;
	NODE parent;
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
		(NODE) getComponent(context, go, COMPONENT_TYPE_NODE)->userData
	);
}

//------------------------------------------------------------------------------
// SPRITE COMPONENT
// Maintains a sprite batch and a draw queue that renders sprites with the 
// node's position.
//------------------------------------------------------------------------------

struct SpriteContext;
struct Sprite;

class SpriteRef {
private:
	SpriteContext *context;
	Sprite *sprite;

	SpriteRef(); // no default ctor

public:
	SpriteRef(SpriteContext *aContext, Sprite *aSprite) : 
		context(aContext), 
		sprite(aSprite) {}

	inline operator Sprite*() const { return sprite; }

	bool visible();
	GoComponent *component();
	ImageAsset *image();
	int frame();
	int layer();
	Color color();

	void setVisible(bool flag);
	void setImage(ImageAsset *asset);
	void setFrame(int frame);
	void setLayer(int layer);
	void setColor(Color c);

	// todo: animation options
};

struct SpriteAsset {
	uint32_t imageHash;
	float x, y;
	Color color;
	int visible;
	int frame;
	int layer;
};

// Create a new sprite context from a given capacity.
SpriteContext *createSpriteContext(AssetBundle *assets, size_t numLayers=8, size_t layerCapacity=128);
void destroy(SpriteContext *context);

// Create a new sprite
// batch methods
void advanceAnimations(SpriteContext *context, float dt);
void draw(SpriteContext *context, SpriteBatch *batch);

// go methods

GoComponentDef spriteDef(SpriteContext *context, AssetBundle *assets);

void addSprite(GoContext *context, GO go, const SpriteAsset *asset=0) {
	addComponent(context, go, COMPONENT_TYPE_SPRITE, asset);
}

inline SpriteRef getSprite(GoContext *context, GO go) {
	return SpriteRef(
		(SpriteContext*) getContext(context, COMPONENT_TYPE_SPRITE),
		(Sprite*) getComponent(context, go, COMPONENT_TYPE_SPRITE)->userData
	);
}
