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
#include "littlepolygon_bitset.h"

// compilation options
#define FK_NO_RECURSION 1

// Implementation Note: I'm still not completely happy with caching
// world transformations per node.  I'm considering some alternatives...
// (i) one dirty bit - setting any transforms dirties the "whole system",
//     and world getters always recompute unless the bit is cleared in 
//     the batch update.
// (ii) dirty buckets - more than one dirty bit, but not a dirty bit for
//      each nodes, but instead a bit in a bucket with some kind of logical
//      hash (parent id?).  Would lead to some false-negatives, but on the 
//      whole would could be balanced and amortized.
// (iii) no cache - just write out to a world-buffer on-demand before other
//       systems use the information.  All node getters would be completely
//       on-demand (separate FK tree for static stuff?)

struct FkNode {
	FkContext *context;
	FkNode *parent;
	FkNode *firstChild;
	FkNode *nextSibling;
	FkNode *prevSibling;
	AffineMatrix local;
	AffineMatrix world;
	void *userData;
	#if FK_NO_RECURSION
	FkNode *unwind;
	#endif
};

struct FkContext {
	size_t capacity;
	size_t count;
	Bitset<1024> allocationMask;
	Bitset<1024> dirtyMask;
	FkNode *firstRoot;	
	FkNode nodebuf[1];

	bool allocated(FkNode *node) {
		ASSERT(node->context == this);
		return allocationMask[node - nodebuf];
	}

	bool dirty(FkNode *node) {
		ASSERT(node->context == this);
		return dirtyMask[node - nodebuf];
	}

	void markDirty(FkNode *node) {
		ASSERT(node->context == this);
		return dirtyMask.mark(node - nodebuf);
	}

	void clearDirty(FkNode *node) {
		ASSERT(node->context == this);
		return dirtyMask.clear(node - nodebuf);
	}

};

FkTreeRef createFkContext(size_t capacity) {
	ASSERT(capacity <= 1024); // because of the bitset mask

	// allocate memory
	auto context = (FkContext*) LITTLE_POLYGON_MALLOC(
		sizeof(FkContext) + 
		sizeof(FkNode) * (capacity-1)
	);

	// intialize fields
	context->capacity = capacity;
	context->count = 0;
	context->allocationMask.reset();
	context->dirtyMask.reset();
	context->firstRoot = 0;
	return context;
}

void FkTreeRef::destroy() {
	LITTLE_POLYGON_FREE(context);
}

FkNodeRef FkTreeRef::addNode(void *userData) {
	ASSERT(context->count < context->capacity);

	unsigned index;
	if (!(~context->allocationMask).findFirst(index)) {
		return 0;
	}

	context->allocationMask.mark(index);
	auto result = context->nodebuf + index;

	// intialize fields
	result->context = context;
	result->parent = 0;
	result->firstChild = 0;
	result->prevSibling = 0;
	result->local = affineIdentity();
	result->userData = userData;

	// attach to the root list
	result->nextSibling = context->firstRoot;
	if (context->firstRoot) { context->firstRoot->prevSibling = result; }
	context->firstRoot = result;

	context->markDirty(result);
	++context->count;
	return result;
}

FkNodeRef FkNodeRef::addNode(void *userData) {
	ASSERT(node->context->count < node->context->capacity);
	auto context = node->context;

	unsigned index;
	if (!(~context->allocationMask).findFirst(index)) {
		return 0;
	}

	context->allocationMask.mark(index);
	auto result = context->nodebuf + index;

	// intialize fields
	result->context = context;
	result->parent = node;
	result->firstChild = 0;
	result->prevSibling = 0;
	result->local = affineIdentity();
	result->userData = userData;

	result->nextSibling = node->firstChild;
	if (node->firstChild) { node->firstChild->prevSibling = result; }
	node->firstChild = result;

	context->markDirty(result);
	++context->count;
	return result;
}

static void doDestroy(FkNode *node) {
	
	// unlink
	if (node->nextSibling) { node->nextSibling->prevSibling = node->prevSibling; }
	if (node->prevSibling) { node->prevSibling->nextSibling = node->nextSibling; }
	if (node->parent) {
		if (node == node->parent->firstChild) { node->parent->firstChild = node->nextSibling; }
	} else {
		if (node->context->firstRoot == node) { node->context->firstRoot = node->nextSibling; }
	}

	// update context
	node->context->allocationMask.clear(node - node->context->nodebuf);
	node->context->dirtyMask.clear(node - node->context->nodebuf);
	--node->context->count;	
	
}

void FkNodeRef::destroy() {

	// kill children bottom->up (no recursive functions)
	FkInvSubtreeIterator iter(node);
	while(!iter.finished()) {
		auto child = iter.current;
		iter.next();
		doDestroy(child);
	}

	doDestroy(node);
}


void FkNodeRef::setParent(FkNodeRef aParent) {
	FkNode *parent = aParent;
	ASSERT(node->context == parent->context);
	auto child = node;
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

void FkNodeRef::detachChildren(bool preserveTransforms) {
	if (preserveTransforms) {
		while(node->firstChild) {
			FkNodeRef(node->firstChild).reparent(0);
		}
	} else {
		while(node->firstChild) {
			FkNodeRef(node->firstChild).setParent(0);
		}
	}
}

void FkNodeRef::setUserData(void *userData) {
	node->userData = userData;
}

void FkNodeRef::setLocal(const AffineMatrix& transform) {
	node->local = transform;
	node->context->markDirty(node);
}

void FkNodeRef::setPosition(vec2 position) {
	node->local.t = position;
	node->context->markDirty(node);
}

void FkNodeRef::setAttitude(vec2 attitude) {
	node->local.u = attitude;
	node->local.v = vec(-attitude.y, attitude.x);
	node->context->markDirty(node);
}

void FkNodeRef::setRotation(float radians) {
	float s = sinf(radians);
	float c = cosf(radians);
	node->local.u = vec(c, s);
	node->local.v = vec(-s, c);
	node->context->markDirty(node);
}

void FkNodeRef::setScale(vec2 scale) {
	node->local.u = vec(scale.x, 0);
	node->local.v = vec(0, scale.y);
	node->context->markDirty(node);
}

static void dirtyChildren(FkNode *node) {
	for(auto p=node->firstChild; p; p=p->nextSibling) {
		p->context->markDirty(p);
	}
}

static bool cacheWorld(FkNode *node) {
	if (node->parent) {

#if FK_NO_RECURSION

		// walk up to parent, saving unwinding-refs back
		node->unwind = 0;
		auto p = node;
		node = node->parent;
		do {
			node->unwind = p;
			p = node;
			node = node->parent;
		} while(node);

		// walk down from parent, looking for the first dirty bit
		node = p;
		while(node && !node->context->dirty(node)) {
			node = node->unwind;
		}

		if (!node) {
			return false;
		}

		while(node) {
			node->world = node->parent->world * node->local;
			node->context->clearDirty(node);
			dirtyChildren(node);
			node = node->unwind;
		}

		return true;

#else
		
		if (cacheWorld(node->parent) || node->context->dirty(node)) {
			node->world = node->parent->world * node->local;
			node->context->clearDirty(node);
			dirtyChildren(node);
			return true;
		} else {
			return false;
		}	

#endif

	} else if (node->context->dirty(node)) {
		node->world = node->local;
		node->context->clearDirty(node);
		dirtyChildren(node);
		return true;
	} else {
		return false;
	}
}

static void doSetWorld(FkNode *node, const AffineMatrix& transform, bool dirtyChildren) {
	if (node->parent) {
		cacheWorld(node->parent);
		node->local = transform * node->parent->world.inverse();
	} else {
		node->local = transform;
	}
	node->world = transform;
	node->context->clearDirty(node);
}

void FkNodeRef::reparent(FkNodeRef aParent) {
	FkNode *parent = aParent;
	if (node->parent != parent) {
		auto worldTransform = world();
		setParent(parent);
		doSetWorld(node, worldTransform, false);	
	}
}

void FkNodeRef::setWorld(const AffineMatrix& transform) {
	doSetWorld(node, transform, true);
	for(auto child=node->firstChild; child; child=child->nextSibling) {
		node->context->markDirty(child);
	}	
}

FkTreeRef FkNodeRef::context() const {
	return node->context;
}

FkNodeRef FkNodeRef::parent() const {
	return node->parent;
}

void* FkNodeRef::userData() const {
	return node->userData;
}

int FkNodeRef::level() const {
	int level = 0;
	auto n = node->parent;
	while(n) {
		++level;
		n = n->parent;
	}
	return level;
}

const AffineMatrix& FkNodeRef::local() const {
	return node->local;
}

vec2 FkNodeRef::position() const {
	return node->local.t;
}

vec2 FkNodeRef::right() const {
	return node->local.u;
}

vec2 FkNodeRef::up() const {
	return node->local.v;
}

const AffineMatrix& FkNodeRef::world() const {
	if (!node->context->dirtyMask.empty()) {
		cacheWorld(node);
	}
	return node->world;
}

//------------------------------------------------------------------------------
// BATCH METHODS
//------------------------------------------------------------------------------

void FkTreeRef::cacheWorldTransforms() {
	// iterate non-recursively through the display tree like it's a
	// foldout gui (children, then siblings, then ancestors)
	auto node = context->firstRoot;
	while(!context->dirtyMask.empty()) {
		if (context->dirty(node)) {
			node->world = node->parent ? 
				node->parent->world * node->local : 
				node->local;
			context->clearDirty(node);
			dirtyChildren(node);
		}
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

const AffineMatrix *FkNodeRef::cachedTransform() const {
	return &node->world;
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

FkChildIterator::FkChildIterator(const FkNode* parent) : 
current(parent->firstChild) {
}

void FkChildIterator::next() {
	current = current->nextSibling;
}

FkTreeIterator::FkTreeIterator(const FkContext* context) : 
current(context->firstRoot) {
}

void FkTreeIterator::next() {
	// first check children, then next siblings, then parents->nextSibling
	if (current->firstChild) {
		current = current->firstChild;
	} else if (current->nextSibling) {
		current = current->nextSibling;
	} else {
		do { current = current->parent; } while (current && !current->nextSibling);
		if (current) { current = current->nextSibling; }
	}
}

FkSubtreeIterator::FkSubtreeIterator(const FkNode *aParent) : 
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

static FkNode *drillDown(FkNode *current) {
	while(current->nextSibling || current->firstChild) {
		if (current->nextSibling) {
			current = current->nextSibling;
		} else if (current->firstChild) {
			current = current->firstChild;
		}
	}
	return current;	
}

FkInvTreeIterator::FkInvTreeIterator(const FkContext *context) {
	// start by drilling down as deep as we can
	current = drillDown(context->firstRoot);
}

void FkInvTreeIterator::next() {
	// we essetentially do the opposite of what the tree
	// iterator does:
	// first check prevSibling's->children, then prevSiblings, then parents
	if (current->prevSibling) {
		current = current->prevSibling;
		if (current->firstChild) {
			current = drillDown(current->firstChild);
		}
	} else if (current->parent) {
		current = current->parent;
	} else {
		current = 0;
	}
}


FkInvSubtreeIterator::FkInvSubtreeIterator(const FkNode *aParent) : 
parent(aParent) {
	current = drillDown((FkNode*)parent);
}

void FkInvSubtreeIterator::next() {
	if (current->prevSibling) {
		current = current->prevSibling;
		if (current->firstChild) {
			current = drillDown(current->firstChild);
		}
	} else if (current->parent != parent) {
		current = current->parent;
	} else {
		current = 0;
	}
}
