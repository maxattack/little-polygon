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
// * E.G. see positionSprites() in gobindings

struct Node {
	Node *parent;
	Node *firstChild;
	Node *nextSibling;
	Node *prevSibling;
	// mat4f local
	void *userData;
};

struct FkContext {
	size_t capacity;
	size_t count;
	Bitset<1024> allocationMask;
	Node *firstRoot;
	
	mat4f first; // really just a hack to make sure SIMD alignment is OK :P

	inline mat4f *tformBuf() { return &first; }
	inline Node *nodeBuf() { return (Node*)(tformBuf() + capacity); }

	bool owns(Node *node) {
		int index = node - nodeBuf();
		return 0 <= index && index < capacity;
	}

	bool allocated(Node *node) {
		return owns(node) && allocationMask[node - nodeBuf()];
	}

	inline mat4f *tform(Node* node) {
		ASSERT(owns(node));
		auto index = node - nodeBuf();
		ASSERT(allocationMask[index]);
		return tformBuf() + index;
	}

};

FkContext *createFkContext(size_t capacity) {
	// in case we wanna use a Bitset<1024> at some point
	ASSERT(capacity <= 1024); 

	// allocate memory
	auto context = (FkContext*) LITTLE_POLYGON_MALLOC(
		sizeof(FkContext) + 
		(capacity-1) * sizeof(mat4f) + 
		(capacity) * sizeof(Node)
	);
	context->capacity = capacity;
	context->count = 0;
	context->allocationMask = Bitset<1024>();
	return context;
}

void destroy(FkContext *context) {
	LITTLE_POLYGON_FREE(context);
}

Node* createNode(FkContext *context, Node* parent, void *userData) {
	ASSERT(context->count < context->capacity);

	unsigned index;
	if (!(~context->allocationMask).clearFirst(index)) {
		return 0;
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
		setParent(context, result, parent);
	} else {
		// attach to the root list
		result->nextSibling = context->firstRoot;
		if (context->firstRoot) { context->firstRoot->prevSibling = result; }
		context->firstRoot = result;
	}

	++context->count;
	return result;
}

void destroy(FkContext *context, Node* node) {
	ASSERT(context->allocated(node));

	// kill children
	while (node->firstChild) {
		destroy(context, node->firstChild);
	}	

	// remove from parent
	setParent(context, node, 0);

	// prepend to free list
	context->allocationMask.clear(node - context->nodeBuf());
	--context->count;
}

void setParent(FkContext *context, Node* child, Node* parent) {
	if (parent) {
		// cleanup existing state
		if (child->parent == parent) { return; }
		if (child->parent) { setParent(context, child, 0); }

		// remove from root list
		if (child->nextSibling) { child->nextSibling->prevSibling = child->prevSibling; }
		if (child->prevSibling) { child->prevSibling->nextSibling = child->nextSibling; }
		if (context->firstRoot == child) { context->firstRoot = child->nextSibling; }

		// add to parent list
		child->nextSibling = parent->firstChild;
		child->prevSibling = 0;
		if (parent->firstChild) { parent->firstChild->prevSibling = child; }
		parent->firstChild = child;
		child->parent = parent;
	} else {
		if (child->parent == 0) { return; }
		// remove from parent list
		if (child->nextSibling) { child->nextSibling->prevSibling = child->prevSibling; }
		if (child->prevSibling) { child->prevSibling->nextSibling = child->nextSibling; }
		if (child == child->parent->firstChild) { child->parent->firstChild = child->nextSibling; }
		child->parent = 0;

		// add to root list
		child->nextSibling = context->firstRoot;
		child->prevSibling = 0;
		if (context->firstRoot) { context->firstRoot->prevSibling = child; }
		context->firstRoot = child;
	}
}

void reparent(FkContext *context, Node* child, Node* parent) {
	if (child->parent != parent) {
		auto worldTransform = world(context, child);
		setParent(context, child, parent);
		setWorld(context, child, worldTransform);	
	}
}

void detachChildren(FkContext *context, Node* node, bool preserveTransforms) {
	if (preserveTransforms) {
		while(node->firstChild) {
			reparent(context, node->firstChild, 0);
		}
	} else {
		while(node->firstChild) {
			setParent(context, node->firstChild, 0);
		}
	}
}

void setUserData(FkContext *context, Node* node, void *userData) {
	node->userData = userData;
}

void setLocal(FkContext *context, Node* node, mat4f transform) {
	*(context->tform(node)) = transform;
}

void setWorld(FkContext *context, Node* node, mat4f transform) {
	if (node->parent) {
		*context->tform(node) = transform * inverse(world(context, node->parent));
	} else {
		*context->tform(node) = transform;
	}
}

Node* parent(FkContext *context, Node* node) {
	return node->parent;
}

mat4f local(FkContext *context, Node* node) {
	return *context->tform(node);
}

mat4f world(FkContext *context, Node* node) {
	if (node->parent) {
		return local(context, node) * world(context, node->parent);
	} else {
		return local(context, node);
	}
}

void* userData(FkContext *context, Node* node) {
	return node->userData;
}

FkChildIterator::FkChildIterator(FkContext *context, Node* parent) : 
current(parent ? parent->firstChild : context->firstRoot) {
}

void FkChildIterator::next() {
	current = current->nextSibling;
}

