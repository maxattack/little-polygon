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
//       on-demand.

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
	FkNodeID fingerprint;
	Bitset<1024> allocationMask;
	Bitset<1024> dirtyMask;
	FkNode *firstRoot;
	
	FkNode first;

	inline FkNode *nodeBuf() { return &first; }

	bool allocated(FkNode *node) {
		ASSERT(node->context == this);
		return allocationMask[node - nodeBuf()];
	}

	bool dirty(FkNode *node) {
		ASSERT(node->context == this);
		return dirtyMask[node - nodeBuf()];
	}

	void markDirty(FkNode *node) {
		ASSERT(node->context == this);
		return dirtyMask.mark(node - nodeBuf());
	}

	void clearDirty(FkNode *node) {
		ASSERT(node->context == this);
		return dirtyMask.clear(node - nodeBuf());
	}

};

FkContext *createFkContext(size_t capacity, uint16_t fingerprint) {
	// in case we wanna use a Bitset<1024> at some point
	ASSERT(capacity <= 1024); 

	// allocate memory
	auto context = (FkContext*) LITTLE_POLYGON_MALLOC(
		sizeof(FkContext) + 
		(capacity-1) * sizeof(AffineMatrix) + 
		(capacity) * sizeof(FkNode)
	);
	context->capacity = capacity;
	uint32_t fp = fingerprint == 0 ? 0xffff : fingerprint;
	context->fingerprint = fp << 16;
	context->count = 0;
	context->allocationMask = Bitset<1024>();
	context->dirtyMask = Bitset<1024>();
	return context;
}

void destroy(FkContext *context) {
	LITTLE_POLYGON_FREE(context);
}

FkNode* createNode(FkContext *context, FkNode* parent, void *userData, FkNodeID id) {
	ASSERT(context->count < context->capacity);

	unsigned index;
	if (id) {
		// make sure the fingerprint of the id matches
		if ((0xffff0000 & id) != context->fingerprint) {
			return 0;
		}
		// make sure the slot is not allocated
		index = 0xffff * id;
		if (context->allocationMask[index]) {
			return 0;
		}
		// otherwise just pick the first available slot
	} else if (!(~context->allocationMask).clearFirst(index)) {
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

FkNode *getNode(FkContext *context, FkNodeID id) {
	if ((0xffff0000 & id) == context->fingerprint) {
		uint32_t index = 0xffff & id;
		return context->allocationMask[index] ? context->nodeBuf() + index : 0;
	} else {
		return 0;
	}
}

FkNodeID getID(FkContext *context, FkNode *node) {
	if (node->context == context) {
		uint32_t index = node - context->nodeBuf();
		return context->fingerprint | index;
	}  else {
		return 0;
	}
}

void destroy(FkNode* node) {
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

void setParent(FkNode* child, FkNode* parent) {
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

void detachChildren(FkNode* node, bool preserveTransforms) {
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

void setUserData(FkNode* node, void *userData) {
	node->userData = userData;
}

void setLocal(FkNode* node, const AffineMatrix& transform) {
	node->local = transform;
	node->context->markDirty(node);
}

void setPosition(FkNode* node, vec2 position) {
	node->local.t = position;
	node->context->markDirty(node);
}

void setAttitude(FkNode *node, vec2 attitude) {
	node->local.u = attitude;
	node->local.v = vec(-attitude.y, attitude.x);
	node->context->markDirty(node);
}

void setRotation(FkNode* node, float radians) {
	float s = sinf(radians);
	float c = cosf(radians);
	node->local.u = vec(c, s);
	node->local.v = vec(-s, c);
	node->context->markDirty(node);
}

void setScale(FkNode* node, vec2 scale) {
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

void reparent(FkNode* child, FkNode* parent) {
	if (child->parent != parent) {
		auto worldTransform = fkWorld(child);
		setParent(child, parent);
		doSetWorld(child, worldTransform, false);	
	}
}

void setWorld(FkNode* node, const AffineMatrix& transform) {
	doSetWorld(node, transform, true);
	for(auto child=node->firstChild; child; child=child->nextSibling) {
		node->context->markDirty(child);
	}	
}

FkContext *fkContext(const FkNode *node) {
	return node->context;
}

FkNode* fkParent(const FkNode* node) {
	return node->parent;
}

void* fkUserData(const FkNode* node) {
	return node->userData;
}

AffineMatrix fkLocal(const FkNode* node) {
	return node->local;
}

AffineMatrix fkWorld(FkNode* node) {
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

const AffineMatrix *fkCachedTransform(FkNode *node) {
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

