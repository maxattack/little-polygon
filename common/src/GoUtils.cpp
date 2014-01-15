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

#include "littlepolygon_go_utils.h"

//------------------------------------------------------------------------------
// FK SYSTEM
//------------------------------------------------------------------------------

int FkSystem::onInit(GoComponent *component, const void *args) {
	
	// unpack args
	auto asset = (const FkNodeAsset*) args;
	FkNodeID id = asset ? asset->id : 0;
	FkNode *parent = asset && asset->parent ? getNode(context, asset->parent) : 0;

	// create node
	FkNode* node = createNode(context, parent, component, id);
	if (asset) {
		setLocal(node, asset->local);
	}

	if (!node) {
		return -1;
	}

	// save as backing store
	setComData(component, node);

	return GOSTATUS_OK;
}

int FkSystem::onEnable(GoComponent *component) {
	
	// enable downtree nodes
	for(FkChildIterator i(coNode(component)); !i.finished(); i.next()) {
		GoComponentRef childComponent = i.ref().data<GoComponent>();
		if (childComponent) {
			GOSTATUS_CHECK( childComponent.gameObject().enable() );
		}
	}
	
	return GOSTATUS_OK;
}

int FkSystem::onMessage(GoComponent *component, int messageId, const void *args) {
	
	// send message downtree
	for(FkChildIterator i(coNode(component)); !i.finished(); i.next()) {
		GoComponentRef childComponent = i.ref().data<GoComponent>();
		if (childComponent) {
			GOSTATUS_CHECK( childComponent.gameObject().sendMessage(messageId, args) );
		}
	}
	
	return GOSTATUS_OK;
}

int FkSystem::onDisable(GoComponent *component) {
	
	// disable downtree nodes
	for(FkChildIterator i(coNode(component)); !i.finished(); i.next()) {
		GoComponentRef childComponent = i.ref().data<GoComponent>();
		if (childComponent) {
			GOSTATUS_CHECK( childComponent.gameObject().disable() );
		}
	}

	return GOSTATUS_OK;
}

int FkSystem::onDestroy(GoComponent *component) {

	// destroy downtree game objects first
	auto iter = FkChildIterator(coNode(component));
	while(!iter.finished()) {
		auto child = iter.ref();
		iter.next();
		auto childComponent = child.data<GoComponent>();
		if (childComponent) {
			destroy( comObject(childComponent) );
		}
	}

	// now destroy meself
	destroy((FkNode*) comData(component));

	return GOSTATUS_OK;
}

//------------------------------------------------------------------------------
// SPRITE SYSTEM
//------------------------------------------------------------------------------

int SpriteSystem::onInit(GoComponent *component, const void *args) {

	// unpack args
	auto asset = (const SpriteAsset *) args;
	ImageAsset *img = asset ? assets->image(asset->imageHash) : 0;
	int frame = asset ? asset->frame : 0;
	Color color = asset ? asset->color : rgba(0);

	// find my node
	GoComponentRef nodeComponent = getComponent(
		comObject(component), 
		LP_COMPONENT_TYPE_NODE
	);
	if (!nodeComponent) {
		return -1;
	}

	auto sprite = createSprite(
		batch, img, 
		fkCachedTransform(nodeComponent.data<FkNode>()),
		frame, color, 0, 0, component
	);

	if (!sprite) {
		return -2;
	}

	// save as backing store
	setComData(component, sprite);

	return GOSTATUS_OK;
}

int SpriteSystem::onEnable(GoComponent *component) {
	coSprite(component).setVisible(1);
	return GOSTATUS_OK;
}

int SpriteSystem::onDisable(GoComponent *component) {
	coSprite(component).setVisible(0);
	return GOSTATUS_OK;
}

int SpriteSystem::onDestroy(GoComponent *component) {
	destroy( (Sprite*) comData(component) );
	return GOSTATUS_OK;
}


