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
#include "littlepolygon_sprites.h"
#include "littlepolygon_collisions.h"

// GO bindings for "core" systems.

struct SpriteAsset {
	uint32_t image;
	int frame;
	Color color;
	int layer;
};

class SpriteComponent : public GoComponentType {
public:

	SpriteComponent(SpriteBatchRef aBatch, AssetRef aAssets) : 
		batch(aBatch), assets(aAssets) {}

	int init(GoComponentRef component, const void *args=0);
	int enable(GoComponentRef component);
	int disable(GoComponentRef component);
	int release(GoComponentRef component);

private:
	SpriteBatchRef batch;
	AssetBundleRef assets;
};

struct ColliderAsset {
	AABB box;
	uint32_t categoryMask
	uint32_t collisionMask;
	uint32_t triggerMask;
	vec2 pivot;
};

class ColliderComponent : public GoComponentType {
public:

	ColliderComponent(CollisionSystemRef aCollisions) : 
		collisions(aCollisions) {}

	int init(GoComponentRef component, const void *args=0);
	int enable(GoComponentRef component);
	int disable(GoComponentRef component);
	int release(GoComponentRef component);

private:
	CollisionSystemRef collisions;
};

struct GoCore {
	AssetsRef assets;
	FkTreeRef nodes;
	SpritePlotterRef plotter;
	SpriteBatchRef batch;
	CollisionSystemRef collisions;
	GoContextRef go;
	SpriteComponent spriteComponent;
	ColliderComponent colliderComponent;

	GoCore(const char *assetPath) : 
		assets(createAssetBundle(assetPath)),
		nodes(createFkContext()),
		plotter(createSpritePlotter()),
		batch(createSpriteBatch()),
		go(createGoContext(nodes)),
		spriteComponent(batch, assets) 
		{}

	~GoCore() {
		go.destroy();
		collisions.destroy();
		batch.destroy();
		plotter.destroy();
		nodes.destroy();
		assets.destroy();
	}	
};

