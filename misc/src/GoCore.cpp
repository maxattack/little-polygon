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

#include "littlepolygon_gocore.h"

int SpriteComponentType::init(GoComponent* component, const void *args) {
	auto *asset = (const SpriteAsset*) args;
	auto sprite = createSprite(
		batch, 
		asset ? assets->image(asset->image) : 0,
		fkCachedTransform(goNode(coObject(component))),
		asset ? asset->frame : 0,
		asset ? asset->color : rgba(0),
		0,
		asset ? asset->layer : 0,
		(void*) component
	);
	if (!sprite) { return -1; }
	setHandle(component, sprite);
	return GOSTATUS_OK;
}

int SpriteComponentType::enable(GoComponent* component) {
	setVisible((Sprite*) coHandle(component), 1);
	return GOSTATUS_OK;
}

int SpriteComponentType::disable(GoComponent* component) {
	setVisible((Sprite*) coHandle(component), 0);
	return GOSTATUS_OK;
}

int SpriteComponentType::release(GoComponent* component) {
	destroy((Sprite*)coHandle(component));
	return GOSTATUS_OK;
}
