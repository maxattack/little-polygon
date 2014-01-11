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
#include "littlepolygon_base.h"
#include "littlepolygon_vmath.h"

// A Generic Forward-Kinematic (FK) system for creating display-trees
// like in flash.  While this is was implemented with the intention of
// integrating with the littlepolygon_go module, it is functional
// independently, and could also be swapped out (e.g. with a simpler
// system).
// When used with a GameObject the userData pointer is reserved by the
// system.
// Implements fully 3D math since even 2D games often have some element
// of "2.5D" and besides, the performance isn't any different and the
// hardware is geared for it anyway.

struct FkContext;
typedef uint32_t NODE;

// Create a context.  Different contexts can be allocated withing a single
// application, but NODEs from different contexts cannot directly
// interact.
FkContext *createFkContext(size_t nodeCapacity=1024);
void destroy(FkContext *context);

// Create a new node whose localToParent transform is initialized to
// the identity matrix.  Optionally initialize with a specific parent
// node and userdata.  The explicit ID argument is useful for serialization
// or network synchronization; the default value of zero will just generate
// a new ID.
NODE createNode(FkContext *context, NODE parent=0, void *userData=0, NODE explicitId=0);

// Destroy this node.  ASSERT()s that we have no children.
void destroy(FkContext *context, NODE node);

// Attach the given child to the given parent, detach from it's current
// parent if necessary.
void setParent(FkContext *context, NODE child, NODE parent=0);

// like addChild, but preserves the localToWorld transform of the child.
void reparent(FkContext *context, NODE child, NODE parent=0);

// if we have any children, detach them all, preserving their local transforms.
void detachChildren(FkContext *context, NODE parent, bool preserveTransforms=false);

// Change the userdata for the given node.
void setUserData(FkContext *context, NODE node, void *userData);

// actually set the transform of the node
void setLocal(FkContext *context, NODE node, mat4 transform);
void setWorld(FkContext *context, NODE node, mat4 transform);
// TODO: specialized versions (e.g. setPosition, setRotation, setScale, etc?)

// Solve the local position of the given world-point, which is often
// cheaper for one-off transforms than actually inverting the localToWorld
// matrix.
vec4 solveLocal(FkContext *context, NODE node, vec4 worldPosition);

// getters
NODE parent(FkContext *context, NODE node);
mat4 local(FkContext *context, NODE node);
mat4 world(FkContext *context, NODE node);
void* userData(FkContext *context, NODE node);

// iterators

struct FkChildIterator {
	void *internal;
	NODE current;

	FkChildIterator(FkContext *context, NODE parent);
	inline bool finished() { return current == 0; }
	void next();
};
