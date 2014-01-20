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

#include "littlepolygon_goedit.h"

#include <string>
#include <littlepolygon_pools.h>

struct EditComponent {
	bool expanded;
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

int EditSystem::init(GoComponent* component, const void *args) {
	auto result = pool.alloc();
	result->expanded = false;
	setHandle(component, result);
	return GOSTATUS_OK;
}

int EditSystem::enable(GoComponent* component) {
	return GOSTATUS_OK;
}

int EditSystem::disable(GoComponent* component) {
	return GOSTATUS_OK;
}

int EditSystem::release(GoComponent* component) {
	pool.release((EditComponent*) coHandle(component));
	return GOSTATUS_OK;
}

bool EditSystem::handleEvents(SDL_Event *event) {
	switch(event->type) {

	}
	return false;
}

void EditSystem::draw(SpritePlotter *plotter) {
	// draw the backdrop
	auto sz = canvasSize(plotter);
	sz.x *= 0.25f;
	drawQuad(plotter, skin.palette, vec(0,0), vec(0,sz.y), vec(sz.x,0), sz);

	// draw the FK heirarchy
	vec2 pos = vec(4,4);
	for(FkTreeIterator i(fkContext(context)); !i.finished(); i.next()) {
		GameObjectRef go = goFromNode(i.current);
		if (go) {
			// pretty lousy string handling, but whatever
			std::string str = go.name();
			int level = go.node().level();
			while(level) {
				str = "  " + str;
				--level;
			}

			drawLabel(plotter, skin.font, pos, rgb(0xffffff), str.c_str());
			pos.y += skin.font->height+1;
		}
	}
}

EditComponent* EditSystem::getEditor(GameObjectRef go) {
	GoComponentRef result = go.getComponent(this);
	if (!result) { result = go.addComponent(this); }
	return result.get<EditComponent>();
}

