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

#include "littlepolygon_gobindings.h"

//------------------------------------------------------------------------------
// NODES
//------------------------------------------------------------------------------

static Node* firstChild(FkContext *context, Node* node) {
	return FkChildIterator(context, node).current;
}

static int nodeInit(GoContext *context, GoComponent *component, const void *args, void *user) {	
	// ?? should parent-child attachments happen *after* init?  It would decouple
	//    potential order-of-initialization issues...

	// type-cast args
	auto fkContext = (FkContext*) user;

	// create backing store
	Node* result;
	if (args) {
		result = createNode(fkContext, 0, component);
	} else {
		auto nodeArgs = (NodeAsset*) args;
		result = createNode(fkContext, 0/*nodeArgs->parent*/, component);

		// initialize geom
		mat4f m;
		m.load(nodeArgs->matrix);
		setLocal(fkContext, result, m);
	}

	// save reference to metadata
	component->userData = (void*) result;

	// everything OK
	return 0;
}

static int nodeDestroy(GoContext *context, GoComponent *component, const void *args, void *user) {	
	

	// typecase args
	auto fkContext = (FkContext*) user;
	auto node = (Node*) component->userData;

	// recursively destroy children
	for (Node* child = firstChild(fkContext, node); child; child=firstChild(fkContext, node)) {
		// ?? will calling destroy within a message like this mess up the GO system ??
		destroy(
			context, 
			((GoComponent*)userData(fkContext, child))->go
		);
	}

	// now destroy this node
	destroy(fkContext, node);

	// everything OK
	return 0;
}

GoComponentDef nodeDef(FkContext *context) {
	// todo: propogate enable/disable
	GoComponentDef result = { context, nodeInit, 0, 0, nodeDestroy };
	return result;
}

//------------------------------------------------------------------------------
// SPRITES
//------------------------------------------------------------------------------

