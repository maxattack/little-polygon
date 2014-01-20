#include "EditSystem.h"
#include <string>

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

