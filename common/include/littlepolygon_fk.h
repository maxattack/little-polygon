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

// this dependency is only required for the go bindings; the system is
// useable without it.
#include "littlepolygon_utils.h"

// A Generic Forward-Kinematic (FK) system for creating display-trees
// like in flash.  While this is was implemented with the intention of
// integrating with the littlepolygon_go module, it is functional
// independently, and could also be swapped out (e.g. with a simpler
// system).
// When used with a GameObject the userData pointer is reserved by the
// system.
// Implements fully 3D math since even 2D games often have some element
// of "2.5D" and besides, the performance isn't any different and the
// hardware is geared for it anyway.  Writing math classes is boring, so
// I found a nice SIMD implementation on github :D

struct FkContext;
struct FkNode;
typedef uint32_t FkNodeID;

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
FkContext *createFkContext(size_t nodeCapacity=1024, uint16_t fingerprint=0);
void destroy(FkContext *context);

// Create a new node whose localToParent transform is initialized to
// the identity matrix.  Optionally initialize with a specific parent
// node and userdata.  The ID field is only necessary for serialization
// and network synchronization.
FkNode* createNode(FkContext *context, FkNode* parent=0, void *userData=0, FkNodeID id=0);

// FkNode IDs are useful for serialization
FkNodeID getID(FkContext *context, FkNode *node);
FkNode *getFkNode(FkContext *context, FkNodeID id);

// Destroy this node and all it's children (with an optional callback if you
// need to know who's being destroyed - invoked in leaf->root order).
void destroy(FkContext *context, FkNode* node);

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
AffineMatrix fkLocal(const FkNode* node);
AffineMatrix fkWorld(FkNode* node);

// This method is buyer-beware!  You can only really be sure that the whole 
// trees world transforms are cached after a call to cacheWorldTransforms().
// The idea is to get up to date, dispatch work to the renderer (e.g. within
// a thread-sync), and then move on.
const AffineMatrix *fkCachedWorld(FkNode *node);

//------------------------------------------------------------------------------
// BATCH METHODS
//------------------------------------------------------------------------------

void cacheWorldTransforms(FkContext *context);

//------------------------------------------------------------------------------
// ITERATORS (redo c++11 style?)
//------------------------------------------------------------------------------

struct FkRootIterator {
	// just top-level root nodes
	FkNode *current;
	FkRootIterator(const FkContext *context);
	inline bool finished() const { return current == 0; }
	void next();
};

struct FkChildIterator {
	// just immediate child nodes
	FkNode* current;
	FkChildIterator(const FkNode* parent);
	inline bool finished() const { return current == 0; }
	void next();
};

struct FkIterator {
	// all nodes, in DAG order
	FkNode *current;
	FkIterator(const FkContext *context);
	inline bool finished() const { return current == 0; }
	void next();
};

struct FkSubtreeIterator {
	// subtree DAG traversal
	const FkNode *parent;
	FkNode *current;
	FkSubtreeIterator(const FkNode *parent);
	inline bool finished() const { return current == 0; }
	void next();
};
