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

// Some reference implementations of some core GO systems.
enum LPComponentType {
	LP_COMPONENT_TYPE_NODE = 1,
	LP_COMPONENT_TYPE_SPRITE = 2
};

//------------------------------------------------------------------------------
// FK DISPLAY TREE SYSTEM
// Endows GameObjects with a position in space and a heirarchical structure.
// Messages are forwarded to children, and GameObjects are disable/destroyed
// with their parents.
//------------------------------------------------------------------------------

// Init args, serializable in asset data.  Content needs to be sorted in parent->child
// order so that we don't get dereferencing errors on the parent field.

struct FkNodeAsset {
	FkNodeID id;
	FkNodeID parent;
	AffineMatrix local;
};

class FkSystem : public GoSystem {
private:
	FkContext *context;

public:
	FkSystem(FkContext *aContext) : 
		GoSystem(LP_COMPONENT_TYPE_NODE), 
		context(aContext) {}

	// Create a new FkNode
	int onInit(GoComponent *component, const void *args);

	// Forward these callbacks downtree
	int onEnable(GoComponent *component);
	int onMessage(GoComponent *component, int messageId, const void *args);
	int onDisable(GoComponent *component);

	// Destroy all gameobjects downtree
	int onDestroy(GoComponent *component);

};

//------------------------------------------------------------------------------
// SPRITE RENDERING SYSTEM
// Endows GameObjects with a rendered sprite, visible when object is enabled.
// Requires a FkNode Component to be attached for positioning.
//------------------------------------------------------------------------------

// Init args, serializable in asset data.  All the content needs to come from 
// one bundle, for now, but I can create an AssetBundleGroup, perhaps, for
// searching for images from multiple sources.

struct SpriteAsset {
	uint32_t imageHash;
	int frame;
	Color color;
};

class SpriteSystem : public GoSystem {
private:
	AssetBundle *assets;
	SpriteBatch *batch;

public:
	SpriteSystem(AssetBundle *assets, SpriteBatch *aBatch) : 
		GoSystem(LP_COMPONENT_TYPE_SPRITE),
		batch(aBatch) {}

	// Create the Sprite
	int onInit(GoComponent *component, const void *args);

	// Show/Hide the Sprite
	int onEnable(GoComponent *component);
	int onDisable(GoComponent *component);

	// Remove from the Batch
	int onDestroy(GoComponent *component);	
};

//------------------------------------------------------------------------------
// COLLISION / PHYSICS SYSTEM
// WIP.  Use my AABB system?  Use Box2D or Chipmunk?
// Component Types: Bodies, Colliders, Joints
//------------------------------------------------------------------------------

// class PhysicsSystem : public GoSystem {
// };

//------------------------------------------------------------------------------
// SCRIPTING SYSTEM
// WIP.  Thinking of using Squirrel (http://squirrel-lang.org)
// Components defined in script, similar to Unity3D.
//------------------------------------------------------------------------------

// struct ScriptAsset {
// 	const char *name;
// };

// class ScriptSystem : public GoSystem {
// };


