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
#include "littlepolygon_pools.h"

// Core GameObject Systems

//------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//------------------------------------------------------------------------------

class SpriteSystem;
class SpriteComponent;
class EditSystem;
class EditComponent;

//------------------------------------------------------------------------------
// SPRITE COMPONENT
//------------------------------------------------------------------------------

struct SpriteAsset {
	uint32_t imageHash;
	int frame;
	Color c;
};

class SpriteComponent : public GoComponent {
friend class SpriteSystem;
public:
	int init();
	int enable();
	int disable();
	int destroy();

	SpriteRef sprite() { return pSprite; }

private:
	SpriteSystem *pSystem;
	Sprite *pSprite;
};

class SpriteSystem {
friend class SpriteComponent;
public:
	SpriteSystem(SpriteBatch *batch, AssetBundle *assets);
	SpriteComponent *addSprite(GameObjectRef go, const SpriteAsset& asset);

private:
	SpriteBatch *batch;
	AssetBundle *assets;
	BitsetPool<SpriteComponent> pool;
};

//------------------------------------------------------------------------------
// EDIT COMPONENT
//------------------------------------------------------------------------------

class EditComponent : public GoComponent {
public:
	int init();
	int enable();
	int disable();
	int destroy();

private:
	EditSystem *pSystem;
	bool collapsed;
};

struct EditSkin {
	FontAsset *font;
	ImageAsset *icons;   // frames for different icons
	ImageAsset *palette; // frames for different colored tiles
};

class EditSystem {
public:
	EditSystem(EditSkin *aSkin) : skin(aSkin) {}

	// Retrieve an edit component for the given gameobject / adding one
	// if none were found.
	EditComponent *edit(GameObjectRef gameObject);

	// If the edit system processed the event return true, otherwise the main
	// thread can continue handling it.
	bool handleEvents(SDL_Event *event);

	// Draw the edit system UI in a batch.
	void draw(SpritePlotter *plotter);

private:
	EditSkin *skin;
	BitsetPool<EditComponent> pool;
};

//------------------------------------------------------------------------------
// SCRIPT COMPONENT
//------------------------------------------------------------------------------

// TODO (squirrel?)

