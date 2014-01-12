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

#include "littlepolygon_fk.h"
#include "littlepolygon_templates.h"

// ? Is there value in caching the world transforms ?
// -- save a "dirty mask" bitset
// -- world() - check all parents' dirty bits (teehee)
// -- ???
// -- profit!

// ? Is it worth is to store transforms in DAG order
//   for batch-processing world transforms (e.g. in a
//   multithreaded universe)
// - Alternatively, we could write nodes out to a buffer
//   in dag order during thread "sync" between physics
//   and rendering.

#define NODE_INDEX(handle) ((0xffff & handle)-1)

struct FkNode {
	NODE node;
	FkNode *parent;
	FkNode *firstChild;
	FkNode *nextSibling;
	FkNode *prevSibling;
	void *userData;
};

struct FkContext {
	size_t capacity;
	size_t count;
	Bitset<1024> allocationMask;
	
	mat4f first; // really just a hack to make sure SIMD alignment is OK :P

	inline mat4f *tformBuf() { return &first; }
	inline FkNode *nodeBuf() { return (FkNode*)(tformBuf() + capacity); }

	inline mat4f *tform(NODE node) {
		ASSERT(node);
		auto index = NODE_INDEX(node);
		ASSERT(index < capacity);
		ASSERT(allocationMask[index]);
		return tformBuf() + index;
	}

	inline FkNode *lookup(NODE node) {
		ASSERT(node);
		auto index = NODE_INDEX(node);
		ASSERT(index < capacity);
		ASSERT(allocationMask[index]);
		return nodeBuf() + index;
	}
};

FkContext *createFkContext(size_t capacity) {
	// in case we wanna use a Bitset<1024> at some point
	ASSERT(capacity <= 1024); 

	// allocate memory
	auto context = (FkContext*) LITTLE_POLYGON_MALLOC(
		sizeof(FkContext) + 
		(capacity-1) * sizeof(mat4f) + 
		(capacity) * sizeof(FkNode)
	);
	context->capacity = capacity;
	context->count = 0;
	context->allocationMask = Bitset<1024>();

	// initialize node indices
	for(int i=0; i<capacity; ++i) {
		context->nodeBuf()[i].node = i+1;
	}

	return 0;
}

void destroy(FkContext *context) {
	LITTLE_POLYGON_FREE(context);
}

NODE createNode(FkContext *context, NODE parent, void *userData, NODE explicitId) {
	ASSERT(context->count < context->capacity);

	unsigned index;
	if (explicitId) {
		index = NODE_INDEX(explicitId);
		ASSERT(index < context->capacity);
		if(context->allocationMask[index]) {
			return 0;
		}
	} else {
		if (!(~context->allocationMask).clearFirst(index)) {
			return 0;
		}
	}

	context->allocationMask.mark(index);
	auto result = context->nodeBuf() + index;

	// intialize fields
	result->parent = 0;
	result->firstChild = 0;
	result->nextSibling = 0;
	result->prevSibling = 0;
	result->userData = userData;

	context->tformBuf()[index] = mat4f::identity();

	if (parent) {
		setParent(context, result->node, parent);
	}

	++context->count;
	return result->node;
}

void destroy(FkContext *context, NODE node) {
	ASSERT(node);
	ASSERT(context->allocationMask[NODE_INDEX(node)]);

	// kill children
	auto slot = context->lookup(node);
	while (slot->firstChild) {
		destroy(context, slot->firstChild->node);
	}	

	// remove from parent
	setParent(context, node, 0);

	// prepend to free list
	slot->node += 0x10000; // fingerprint slot
	context->allocationMask.clear(NODE_INDEX(node));
	--context->count;
}

void setParent(FkContext *context, NODE child, NODE parent) {
	auto childNode = context->lookup(child);

	if (parent) {
		auto parentNode = context->lookup(parent);
		// cleanup existing state
		if (childNode->parent == parentNode) { return; }
		if (childNode->parent) { setParent(context, child, 0); }
		// add to linked list
		childNode->nextSibling = parentNode->firstChild;
		if (parentNode->firstChild) { parentNode->firstChild->prevSibling = childNode; }
		parentNode->firstChild = childNode;
		childNode->parent = parentNode;
	} else {
		if (childNode->parent == 0) { return; }
		// remove from linked list
		if (childNode->nextSibling) { childNode->nextSibling->prevSibling = childNode->prevSibling; }
		if (childNode->prevSibling) { childNode->prevSibling->nextSibling = childNode->nextSibling; }
		if (childNode == childNode->parent->firstChild) { childNode->parent->firstChild = childNode->nextSibling; }
		childNode->nextSibling = 0;
		childNode->prevSibling = 0;
		childNode->parent = 0;
	}
}

void reparent(FkContext *context, NODE child, NODE parent) {
	// check first for NOOP
	auto childNode = context->lookup(child);
	if (parent) {
		if (childNode->parent == context->lookup(parent)) { return; }
	} else {
		if(childNode->parent == 0) { return; }
	}

	// if we didn't bail early, then actually do stuff
	auto worldTransform = world(context, child);
	setParent(context, child, parent);
	setWorld(context, child, worldTransform);	
}

void detachChildren(FkContext *context, NODE parent, bool preserveTransforms) {
	auto node = context->lookup(parent);
	if (preserveTransforms) {
		while(node->firstChild) {
			reparent(context, node->firstChild->node, 0);
		}
	} else {
		while(node->firstChild) {
			setParent(context, node->firstChild->node, 0);
		}
	}
}

void setUserData(FkContext *context, NODE node, void *userData) {
	context->lookup(node)->userData = userData;
}

void setLocal(FkContext *context, NODE node, mat4f transform) {
	*(context->tform(node)) = transform;
}

void setWorld(FkContext *context, NODE node, mat4f transform) {
	auto slot = context->lookup(node);
	if (slot->parent) {
		*context->tform(node) = transform * inverse(world(context, slot->parent->node));
	} else {
		*context->tform(node) = transform;
	}
}

NODE parent(FkContext *context, NODE node) {
	return context->lookup(node)->parent->node;
}

mat4f local(FkContext *context, NODE node) {
	return *context->tform(node);
}

mat4f world(FkContext *context, NODE node) {
	auto slot = context->lookup(node);
	if (slot->parent) {
		return local(context, node) * world(context, slot->parent->node);
	} else {
		return local(context, node);
	}
}

void* userData(FkContext *context, NODE node) {
	return context->lookup(node)->userData;
}

FkChildIterator::FkChildIterator(FkContext *context, NODE parent) : 
internal(context->lookup(parent)->firstChild),
current(internal ? ((FkNode*)internal)->node : 0) {
}

void FkChildIterator::next() {
	internal = ((FkNode*)internal)->nextSibling;
	current = internal ? ((FkNode*)internal)->node : 0;
}

//------------------------------------------------------------------------------
// GO SYSTEM BINDINGS
//------------------------------------------------------------------------------

static int nodeMessageHandler(
	GoComponent *component, 
	int msg, 
	const void *args, 
	void *user
) {	
	auto fkContext = (FkContext*) user;
	
	switch(msg) {
		case GO_MESSAGE_INIT: {

			// should parent-child attachments happen *after* init?  It would decouple
			// potential order-of-initialization issues...
			auto nodeArgs = (NodeAsset*) args;
			NODE result = createNode(fkContext, nodeArgs->parent, component, nodeArgs->id);
			mat4f m;
			m.load(nodeArgs->matrix);
			setLocal(fkContext, result, m);
			component->userData = (void*) result;
			return 0;

		}
		
		case GO_MESSAGE_DESTROY: {
			
			// first destroy all the game objects of all our children
			// (this will recursively destroy their children, etc)
			// ?? will calling destroy within a message like this mess up
			//    the GO system ??
			auto node = (NODE) component->userData;
			auto slot = fkContext->lookup(node);
			while(slot->firstChild) {
				auto childComponent = (GoComponent*) slot->firstChild->userData;
				destroy(childComponent->context, childComponent->go);
			}

			// now destroy this node
			destroy(fkContext, node);

			return 0;
		}

		default: return 0;
	}
}

GoComponentDef nodeDef(FkContext *context) {
	GoComponentDef result = { context, nodeMessageHandler };
	return result;
}

