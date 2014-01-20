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

#include <littlepolygon_go_core.h>
#include <littlepolygon_pools.h>

struct EditComponent {
	bool expanded;
};

struct EditSkin {
	FontAsset *font;
	ImageAsset *icons;
	ImageAsset *palette;
};

class EditSystem : GoComponentType {
public:
	EditSystem(GoContext *aContext, const EditSkin& aSkin) : 
		context(aContext), skin(aSkin) {}
	virtual ~EditSystem() {}

	int init(GoComponent* component, const void *args=0);
	int enable(GoComponent* component);
	int disable(GoComponent* component);
	int release(GoComponent* component);

	bool handleEvents(SDL_Event *event);
	void draw(SpritePlotter *batch);

private:
	GoContext *context;
	EditSkin skin;
	FreelistPool<EditComponent> pool;

	EditComponent *getEditor(GameObjectRef go);
};