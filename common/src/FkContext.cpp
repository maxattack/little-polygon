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

// This implementation is pretty naive right now.  I'd like to store
// nodes as a structure-of-arrays that's sorted in DAG order so that 
// we can get a batched interface that's fast for computing the whole
// concatenated tree, but I'm waiting for an actual concrete use case
// to inform the fidgety details.

#define NODE_INDEX(handle) ((0xffff & handle)-1)

struct FkNode {
	NODE node;
	FkNode *parent;
	FkNode *firstChild;
	FkNode *nextSibling;
	FkNode *prevSibling;
	void *userData;

	union {
		uint32_t flags;
		struct {
			uint32_t allocated : 1;
			uint32_t unused : 31;
		};
	};

	mat4 localToParent;
};

struct FkContext {
	size_t capacity;
	FkNode *firstFree;
	FkNode first; // really just a hack to make sure SIMD alignment is OK :P

	inline FkNode *nodeBuf() { return &first; }

	inline FkNode *lookup(NODE node) {
		auto index = NODE_INDEX(node);
		ASSERT(index < capacity);
		auto result = nodeBuf() + index;
		ASSERT(result->allocated);
		return result;
	}
};

FkContext *createFkContext(size_t capacity) {
	// in case we wanna use a Bitset<1024> at some point
	ASSERT(capacity <= 1024); 

	// allocate memory
	auto context = (FkContext*) LITTLE_POLYGON_MALLOC(
		sizeof(FkContext) + 
		(capacity-1) * sizeof(FkNode)
	);
	context->capacity = capacity;

	// initialize the free-list
	auto nodes = context->nodeBuf();
	context->firstFree = nodes;
	for(size_t i=0; i<capacity; ++i) {
		nodes[i].flags = 0;
		nodes[i].node = i+1;
		nodes[i].nextSibling = i < capacity-1 ? &nodes[i+1] : 0;
		nodes[i].prevSibling = i > 0 ? &nodes[i-0] : 0;
	}

	return 0;
}

void destroy(FkContext *context) {
	LITTLE_POLYGON_FREE(context);
}

NODE createNode(FkContext *context, NODE parent, void *userData, NODE explicitId) {
	ASSERT(context->firstFree);

	auto result = context->firstFree;
	if (explicitId) {
		auto index = NODE_INDEX(explicitId);
		ASSERT(index < context->capacity);
		result = context->nodeBuf() + index;
		if (result->allocated) {
			return 0;
		}
	} else {
		result = context->firstFree;
	}

	// pop first node from free list
	if (result == context->firstFree) {
		context->firstFree = context->firstFree->nextSibling;
	}

	// intialize fields
	result->parent = 0;
	result->firstChild = 0;
	result->nextSibling = 0;
	result->prevSibling = 0;
	result->userData = userData;
	result->allocated = 1;
	result->localToParent = mat();

	if (parent) {
		setParent(context, result->node, parent);
	}

	return result->node;
}

void destroy(FkContext *context, NODE node, FkNodeCallback willDestroy) {
	// kill children
	auto slot = context->lookup(node);
	while (slot->firstChild) {
		destroy(context, slot->firstChild->node, willDestroy);
	}	

	if (willDestroy) {
		willDestroy(node);
	}

	// remove from parent
	setParent(context, node, 0);

	// prepend to free list
	slot->nextSibling = context->firstFree;
	slot->prevSibling = 0;
	slot->flags = 0;
	slot->node += 0x10000; // fingerprint slot
	if (context->firstFree) { context->firstFree->prevSibling = slot; }
	context->firstFree = slot;
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

void setLocal(FkContext *context, NODE node, mat4 transform) {
	auto slot = context->lookup(node);
	slot->localToParent = transform;
}

void setWorld(FkContext *context, NODE node, mat4 transform) {
	auto slot = context->lookup(node);
	if (slot->parent) {
		slot->localToParent = transform * invert(world(context, slot->parent->node));
	} else {
		slot->localToParent = transform;
	}
}

// vec4 solveLocal(FkContext *context, NODE node, vec4 worldPosition) {
// 	// temp
// 	return invert(world(context, node)) * worldPosition;
// }

NODE parent(FkContext *context, NODE node) {
	return context->lookup(node)->parent->node;
}

mat4 local(FkContext *context, NODE node) {
	return context->lookup(node)->localToParent;
}

mat4 world(FkContext *context, NODE node) {
	auto slot = context->lookup(node);
	if (slot->parent) {
		return slot->localToParent * world(context, slot->parent->node);
	} else {
		return slot->localToParent;
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
