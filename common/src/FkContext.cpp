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

	bool owns(Node *node) {
		auto index = node - nodeBuf();
		return 0 <= index && index < capacity;
	}

	bool allocated(Node *node) {
		ASSERT(owns(node));
		return allocationMask[node - nodeBuf()];
	}

	bool dirty(Node *node) {
		ASSERT(owns(node));
		return dirtyMask[node - nodeBuf()];
	}

	void markDirty(Node *node) {
		ASSERT(owns(node));
		return dirtyMask.mark(node - nodeBuf());
	}

	void clearDirty(Node *node) {
		ASSERT(owns(node));
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
	result->parent = 0;
	result->firstChild = 0;
	result->nextSibling = 0;
	result->prevSibling = 0;
	result->local = affineIdentity();
	result->world = affineIdentity();
	result->userData = userData;

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
	context->dirtyMask.clear(node - context->nodeBuf());
	--context->count;
}

void setParent(FkContext *context, Node* child, Node* parent) {
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

void reparent(FkContext *context, Node* child, Node* parent) {
	if (child->parent != parent) {
		auto worldTransform = world(child);
		setParent(context, child, parent);
		setWorld(child, worldTransform);	
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

void setUserData(Node* node, void *userData) {
	node->userData = userData;
}

void setLocal(FkContext *context, Node* node, AffineMatrix transform) {
	node->local = transform;
	context->markDirty(node);
}

static bool cacheWorld(FkContext *context, Node *node) {
	if (node->parent) {
		if (cacheWorld(context, node->parent) || context->dirty(node)) {
			node->world = node->parent->world * node->local;
			context->clearDirty(node);
			return true;
		} else {
			return false;
		}
	} else if (context->dirty(node)) {
		node->world = node->local;
		context->clearDirty(node);
		return true;
	} else {
		return false;
	}
}

void setWorld(FkContext *context, Node* node, AffineMatrix transform) {
	if (node->parent) {
		cacheWorld(context, node->parent);
		node->local = transform * node->parent->world.inverse();
	} else {
		node->local = transform;
	}
	node->world = transform;
	context->clearDirty(node);
	for(auto child=node->firstChild; child; child=child->nextSibling) {
		context->markDirty(child);
	}
}

Node* parent(Node* node) {
	return node->parent;
}

AffineMatrix local(Node* node) {
	return node->local;
}

AffineMatrix world(FkContext *context, Node* node) {
	if (!context->dirtyMask.empty()) {
		cacheWorld(context, node);
	}
	return node->world;
}

void* userData(Node* node) {
	return node->userData;
}

void cacheWorldTransforms(FkContext *context) {
	// iterate non-recursively through the display tree like it's a
	// foldout gui (children, then siblings, then ancestors)
	auto node = context->firstRoot;
	while(!context->dirtyMask.empty()) {
		cacheWorld(context, node);
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

FkChildIterator::FkChildIterator(FkContext *context, Node* parent) : 
current(parent ? parent->firstChild : context->firstRoot) {
}

void FkChildIterator::next() {
	current = current->nextSibling;
}

