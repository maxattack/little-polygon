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

#pragma once

#include "littlepolygon_math.h"

// A Generic Forward-Kinematic (FK) system for creating display-trees
// like in flash.  While this is was implemented with the intention of
// integrating with the littlepolygon_go module, it is functional
// independently, and could also be swapped out (e.g. with a simpler
// system).
// When used with a GameObject the userData pointer is reserved by the
// system.

#ifndef FK_CAPACITY
#define FK_CAPACITY 1024
#endif

struct FkContext;
struct FkNode;

// Gift Ideas:
// (i) Shared Backing-Store for World Transforms?
// (ii) Alternatively, support for copying a world tform buffer?  E.g.
//      int fkWordlIndex(Node *node);
//      void computeWorldTransforms(FkContext *context, AffineMatrix *outResult);

//------------------------------------------------------------------------------
// CONTEXT
//------------------------------------------------------------------------------

// Create a context.  Different contexts can be allocated withing a single
// application, but FkNode*s from different contexts cannot directly
// interact.
FkContext *createFkContext();
void destroy(FkContext *context);

// Create a new node whose localToParent transform is initialized to
// the identity matrix.  Optionally initialize with a specific parent
// node and userdata.  The ID field is only necessary for serialization
// and network synchronization.
FkNode* createNode(FkContext *context, FkNode* parent=0, void *userData=0);

void destroy(FkNode* node);

//------------------------------------------------------------------------------
// HEIRARCHY
//------------------------------------------------------------------------------

// Attach the given child to the given parent, detach from it's current
// parent if necessary.
void setParent(FkNode* child, FkNode* parent=0);

// like addChild, but preserves the localToWorld transform of the child.
void reparent(FkNode* child, FkNode* parent=0);

// if we have any children, detach them all, preserving their local transforms.
void detachChildren(FkNode* parent, bool preserveTransforms=false);

//------------------------------------------------------------------------------
// SETTERS
//------------------------------------------------------------------------------

void setLocal(FkNode* node, const AffineMatrix& transform);
void setPosition(FkNode* node, vec2 position);
void setAttitude(FkNode *node, vec2 attitude);
void setRotation(FkNode* node, float radians);
void setScale(FkNode* node, vec2 scale);
void setWorld(FkNode* node, const AffineMatrix& transform);
void setUserData(FkNode* node, void *userData);

//------------------------------------------------------------------------------
// GETTERS
//------------------------------------------------------------------------------

FkContext *fkContext(const FkNode *node);
FkNode* fkParent(const FkNode* node);
void* fkUserData(const FkNode* node);
int fkLevel(const FkNode *node);
const AffineMatrix& fkLocal(const FkNode* node);
const AffineMatrix& fkWorld(FkNode* node);

//------------------------------------------------------------------------------
// CACHE METHODS
//------------------------------------------------------------------------------

//
// Cached transforms are buyer-beware!  You can only really be sure that
// world transforms are cached after a call to cacheWorldTransforms(), e.g:
// 
//    scene.entity.sprite = createSprite(batch, img, fkCachedWorld(node));
//    ...
//    scene.tick();
//    cacheWorldTransforms(nodes);
//    scene.draw();
//

void cacheWorldTransforms(FkContext *context);
const AffineMatrix *fkCachedTransform(FkNode *node);

//------------------------------------------------------------------------------
// C++ INTERFACE
//------------------------------------------------------------------------------

class FkNodeRef {
private:
	FkNode *node;

public:
	FkNodeRef() {}
	FkNodeRef(FkNode *aNode) : node(aNode) {}

	operator FkNode*() { return node; }
	operator bool() const { return node != 0; }

	void setParent(FkNodeRef parent=0) { ::setParent(node, parent); }
	void reparent(FkNodeRef parent=0) { ::reparent(node, parent); }
	void detachChildren(bool preserveTransforms=false) { ::detachChildren(node, preserveTransforms); }

	void setLocal(const AffineMatrix& matrix) { ::setLocal(node, matrix); }
	void setPosition(vec2 position) { ::setPosition(node, position); }
	void setPosition(float x, float y) { ::setPosition(node, vec(x,y)); }
	void setAttitude(vec2 attitude) { ::setAttitude(node, attitude); }
	void setAttitude(float x, float y) { ::setAttitude(node, vec(x,y)); }
	void setRotation(float radians) { ::setRotation(node, radians); }
	void setScale(vec2 scale) { ::setScale(node, scale); }
	void setScale(float x, float y) { ::setScale(node, vec(x,y)); }
	void setWorld(const AffineMatrix& matrix) { ::setWorld(node, matrix); }
	void setUserData(void *userData) { ::setUserData(node, userData); }

	void apply(const AffineMatrix& matrix) { ::setLocal(node, fkLocal(node) * matrix); }

	FkContext *context() const { return fkContext(node); }
	FkNodeRef parent() const { return fkParent(node); }
	int level() const { return fkLevel(node); }
	const AffineMatrix& local() const { return fkLocal(node); }
	vec2 position() const { return fkLocal(node).t; }
	vec2 right() const { return fkLocal(node).u; }
	vec2 up() const { return fkLocal(node).v; }
	const AffineMatrix& world() const { return fkWorld(node); }
	const AffineMatrix* cachedTransform() const { return fkCachedTransform(node); }

	template<typename T>
	T *data() const { return (T*) fkUserData(node); }

	void destroy() { ::destroy(node); }
};


//------------------------------------------------------------------------------
// ITERATORS (redo c++11 style?)
//------------------------------------------------------------------------------

// all nodes, in DAG order
struct FkTreeIterator {
	FkNode *current;

	FkTreeIterator(const FkContext *context);
	bool finished() const { return current == 0; }
	FkNodeRef ref() { return current; }
	void next();
};

// just top-level root nodes
struct FkRootIterator {
	FkNode *current;
	
	FkRootIterator(const FkContext *context);
	bool finished() const { return current == 0; }
	FkNodeRef ref() { return current; }
	void next();
};

// just immediate child nodes
struct FkChildIterator {
	FkNode *current;

	FkChildIterator(const FkNode* parent);
	bool finished() const { return current == 0; }
	FkNodeRef ref() { return current; }
	void next();
};

// subtree DAG traversal
struct FkSubtreeIterator {
	const FkNode *parent;
	FkNode *current;
	
	FkSubtreeIterator(const FkNode *parent);
	bool finished() const { return current == 0; }
	FkNodeRef ref() { return current; }
	void next();
};

// all nodes, in inverse-DAG order
struct FkInvTreeIterator {
	FkNode *current;

	FkInvTreeIterator(const FkContext *context);
	bool finished() const { return current == 0; }
	FkNodeRef ref() { return current; }
	void next();	
};

// subtree, inverse-DAG order
struct FkInvSubtreeIterator {
	const FkNode *parent;
	FkNode *current;
	
	FkInvSubtreeIterator(const FkNode *parent);
	bool finished() const { return current == 0; }
	FkNodeRef ref() { return current; }
	void next();
};



