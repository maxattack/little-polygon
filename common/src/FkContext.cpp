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

struct Node {
	FkContext *context;
	Node *parent;
	Node *firstChild;
	Node *nextSibling;
	Node *prevSibling;
	AffineMatrix local;
	AffineMatrix world;
	void *userData;
};

struct FkContext {
	size_t capacity;
	size_t count;
	Bitset<1024> allocationMask;
	Bitset<1024> dirtyMask;
	Node *firstRoot;
	
	Node first;

	inline Node *nodeBuf() { return &first; }

	bool allocated(Node *node) {
		ASSERT(node->context == this);
		return allocationMask[node - nodeBuf()];
	}

	bool dirty(Node *node) {
		ASSERT(node->context == this);
		return dirtyMask[node - nodeBuf()];
	}

	void markDirty(Node *node) {
		ASSERT(node->context == this);
		return dirtyMask.mark(node - nodeBuf());
	}

	void clearDirty(Node *node) {
		ASSERT(node->context == this);
		return dirtyMask.clear(node - nodeBuf());
	}

};

FkContext *createFkContext(size_t capacity) {
	// in case we wanna use a Bitset<1024> at some point
	ASSERT(capacity <= 1024); 

	// allocate memory
	auto context = (FkContext*) LITTLE_POLYGON_MALLOC(
		sizeof(FkContext) + 
		(capacity-1) * sizeof(AffineMatrix) + 
		(capacity) * sizeof(Node)
	);
	context->capacity = capacity;
	context->count = 0;
	context->allocationMask = Bitset<1024>();
	context->dirtyMask = Bitset<1024>();
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
	result->context = context;
	result->parent = parent;
	result->firstChild = 0;
	result->prevSibling = 0;
	result->local = affineIdentity();
	result->userData = userData;

	if (parent) {
		
		result->nextSibling = parent->firstChild;
		if (parent->firstChild) { parent->firstChild->prevSibling = result; }
		parent->firstChild = result;

	} else {

		// attach to the root list
		result->nextSibling = context->firstRoot;
		if (context->firstRoot) { context->firstRoot->prevSibling = result; }
		context->firstRoot = result;

	}

	context->markDirty(result);
	++context->count;
	return result;
}

void destroy(Node* node) {
	auto context = node->context;

	// kill children
	while (node->firstChild) {
		destroy(node->firstChild);
	}	

	// remove from parent
	setParent(node, 0);

	// prepend to free list
	context->allocationMask.clear(node - context->nodeBuf());
	context->dirtyMask.clear(node - context->nodeBuf());
	--context->count;
}

void setParent(Node* child, Node* parent) {
	ASSERT(child->context == parent->context);
	auto context = child->context;

	// check for noop
	if (child->parent == parent) { return; }
	
	// unlink
	if (child->nextSibling) { child->nextSibling->prevSibling = child->prevSibling; }
	if (child->prevSibling) { child->prevSibling->nextSibling = child->nextSibling; }
	if (child->parent) {
		if (child == child->parent->firstChild) { child->parent->firstChild = child->nextSibling; }
	} else {
		if (context->firstRoot == child) { context->firstRoot = child->nextSibling; }
	}

	if (parent) {
		// add to parent list
		child->nextSibling = parent->firstChild;
		child->prevSibling = 0;
		if (parent->firstChild) { parent->firstChild->prevSibling = child; }
		parent->firstChild = child;
	} else {
		// add to root list
		child->nextSibling = context->firstRoot;
		child->prevSibling = 0;
		if (context->firstRoot) { context->firstRoot->prevSibling = child; }
		context->firstRoot = child;
	}

	child->parent = parent;
	context->markDirty(child);
}

void reparent(Node* child, Node* parent) {
	if (child->parent != parent) {
		auto worldTransform = world(child);
		setParent(child, parent);
		setWorld(child, worldTransform);	
	}
}

void detachChildren(Node* node, bool preserveTransforms) {
	if (preserveTransforms) {
		while(node->firstChild) {
			reparent(node->firstChild, 0);
		}
	} else {
		while(node->firstChild) {
			setParent(node->firstChild, 0);
		}
	}
}

void setUserData(Node* node, void *userData) {
	node->userData = userData;
}

void setLocal(Node* node, AffineMatrix transform) {
	node->local = transform;
	node->context->markDirty(node);
}

void setPosition(Node* node, vec2 position) {
	node->local.t = position;
	node->context->markDirty(node);
}

void setAttitude(Node *node, vec2 attitude) {
	node->local.u = attitude;
	node->local.v = vec(-attitude.y, attitude.x);
	node->context->markDirty(node);
}

void setRotation(Node* node, float radians) {
	float s = sinf(radians);
	float c = cosf(radians);
	node->local.u = vec(c, s);
	node->local.v = vec(-s, c);
	node->context->markDirty(node);
}

void setScale(Node* node, vec2 scale) {
	node->local.u = vec(scale.x, 0);
	node->local.v = vec(0, scale.y);
	node->context->markDirty(node);
}

static bool cacheWorld(Node *node) {
	if (node->parent) {
		if (cacheWorld(node->parent) || node->context->dirty(node)) {
			node->world = node->parent->world * node->local;
			node->context->clearDirty(node);
			return true;
		} else {
			return false;
		}
	} else if (node->context->dirty(node)) {
		node->world = node->local;
		node->context->clearDirty(node);
		return true;
	} else {
		return false;
	}
}

void setWorld(Node* node, AffineMatrix transform) {
	if (node->parent) {
		cacheWorld(node->parent);
		node->local = transform * node->parent->world.inverse();
	} else {
		node->local = transform;
	}
	node->world = transform;
	node->context->clearDirty(node);
	for(auto child=node->firstChild; child; child=child->nextSibling) {
		node->context->markDirty(child);
	}
}

Node* parent(const Node* node) {
	return node->parent;
}

void* userData(const Node* node) {
	return node->userData;
}

AffineMatrix local(const Node* node) {
	return node->local;
}

AffineMatrix world(Node* node) {
	if (!node->context->dirtyMask.empty()) {
		cacheWorld(node);
	}
	return node->world;
}

//------------------------------------------------------------------------------
// BATCH METHODS
//------------------------------------------------------------------------------

void cacheWorldTransforms(FkContext *context) {
	// iterate non-recursively through the display tree like it's a
	// foldout gui (children, then siblings, then ancestors)
	auto node = context->firstRoot;
	while(!context->dirtyMask.empty()) {
		cacheWorld(node);
		if (node->firstChild) {
			node = node->firstChild;
		} else if (node->nextSibling) {
			node = node->nextSibling;
		} else {
			do { node = node->parent; } while (node && !node->nextSibling);
			if (node) { node = node->nextSibling; }
		}
	}
}

//------------------------------------------------------------------------------
// ITERATORS
//------------------------------------------------------------------------------

FkRootIterator::FkRootIterator(const FkContext* context) : 
current(context->firstRoot) {
}

void FkRootIterator::next() {
	current = current->nextSibling;
}

FkChildIterator::FkChildIterator(const Node* parent) : 
current(parent->firstChild) {
}

void FkChildIterator::next() {
	current = current->nextSibling;
}

FkIterator::FkIterator(const FkContext* context) : 
current(context->firstRoot) {
}

void FkIterator::next() {
	if (current->firstChild) {
		current = current->firstChild;
	} else if (current->nextSibling) {
		current = current->nextSibling;
	} else {
		do { current = current->parent; } while (current && !current->nextSibling);
		if (current) { current = current->nextSibling; }
	}
}

FkSubtreeIterator::FkSubtreeIterator(const Node *aParent) : 
parent(aParent), current(aParent->firstChild) {
}

void FkSubtreeIterator::next() {
	if (current->firstChild) {
		current = current->firstChild;
	} else if (current->nextSibling) {
		current = current->nextSibling;
	} else {
		do { current = current->parent; } while (current != parent && !current->nextSibling);
		if (current == parent) {
			current = 0;
		} else {
			current = current->nextSibling; 
		}
	}
}

