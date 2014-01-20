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


// Extent Core systems with Go Bindsings

struct SpriteAsset {
	uint32_t image;
	int frame;
	Color color;
	int layer;
};

class SpriteComponentType : public GoComponentType {
public:

	SpriteComponentType(SpriteBatch *aBatch, AssetBundle *aAssets) : 
		batch(aBatch), assets(aAssets) {}

	int init(GoComponent* component, const void *args=0);
	int enable(GoComponent* component);
	int disable(GoComponent* component);
	int release(GoComponent* component);

private:
	SpriteBatch *batch;
	AssetBundle *assets;
};

struct GoCore {
	AssetBundle *assets;
	FkContext *nodes;
	SpritePlotter *plotter;
	SpriteBatch *batch;
	GoContext *go;
	SpriteComponentType spriteComponent;

	GoCore(const char *assetPath) : 
		assets(createAssetBundle(assetPath)),
		nodes(createFkContext()),
		plotter(createSpritePlotter()),
		batch(createSpriteBatch()),
		go(createGoContext(nodes)),
		spriteComponent(batch, assets) 
		{}

	~GoCore() {
		destroy(go);
		destroy(batch);
		destroy(plotter);
		destroy(nodes);
		destroy(assets);		
	}
};

