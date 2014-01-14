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
struct Node;

// Create a context.  Different contexts can be allocated withing a single
// application, but Node*s from different contexts cannot directly
// interact.
FkContext *createFkContext(size_t nodeCapacity=1024);
void destroy(FkContext *context);

// Create a new node whose localToParent transform is initialized to
// the identity matrix.  Optionally initialize with a specific parent
// node and userdata.  
Node* createNode(FkContext *context, Node* parent=0, void *userData=0);

// Destroy this node and all it's children (with an optional callback if you
// need to know who's being destroyed - invoked in leaf->root order).
void destroy(FkContext *context, Node* node);

// Attach the given child to the given parent, detach from it's current
// parent if necessary.
void setParent(FkContext *context, Node* child, Node* parent=0);

// like addChild, but preserves the localToWorld transform of the child.
void reparent(FkContext *context, Node* child, Node* parent=0);

// if we have any children, detach them all, preserving their local transforms.
void detachChildren(FkContext *context, Node* parent, bool preserveTransforms=false);

// Change the userdata for the given node.
void setUserData(Node* node, void *userData);

// actually set the transform of the node
void setLocal(FkContext *context, Node* node, AffineMatrix transform);
void setWorld(FkContext *context, Node* node, AffineMatrix transform);
// TODO: specialized versions (e.g. setPosition, setRotation, setScale, etc?)

// getters
Node* parent(Node* node);
AffineMatrix local(FkContext *context, Node* node);
AffineMatrix world(FkContext *context, Node* node);
void* userData(Node* node);

// iterators

struct FkChildIterator {
	Node* current;

	FkChildIterator(FkContext *context, Node* parent=0);
	inline bool finished() const { return current == 0; }
	void next();
};
