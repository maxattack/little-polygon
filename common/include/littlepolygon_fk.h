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



// Gift Ideas:
// (i) Shared Backing-Store for World Transforms?
// (ii) Alternatively, support for copying a world tform buffer?  E.g.
//      int fkWordlIndex(Node *node);
//      void computeWorldTransforms(FkContext *context, AffineMatrix *outResult);

//------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//------------------------------------------------------------------------------

struct FkContext;
struct FkNode;
class FkTreeRef;
class FkNodeRef;

//------------------------------------------------------------------------------
// CONTEXT
//------------------------------------------------------------------------------

// Create a context.  Different contexts can be allocated withing a single
// application, but FkNodeRefs from different contexts cannot directly
// interact.
FkContext *createFkContext(size_t capacity=1024);

class FkTreeRef {
private:
	FkContext *context;

public:
	FkTreeRef() {}
	FkTreeRef(FkContext *aContext) : context(aContext) {}

	operator FkContext*() { return context; }
	operator bool() const { return context != 0; }

	FkNodeRef addNode(void *userData=0);

	template<typename T>
	FkNodeRef addNode(T *data=0);

	void cacheWorldTransforms();
	void destroy();
};

//------------------------------------------------------------------------------
// NODES
//------------------------------------------------------------------------------

class FkNodeRef {
private:
	FkNode *node;

public:
	FkNodeRef() {}
	FkNodeRef(FkNode *aNode) : node(aNode) {}

	operator FkNode*() { return node; }
	operator bool() const { return node != 0; }

	FkNodeRef addNode(void *userData=0);

	template<typename T>
	FkNodeRef addNode(T *data=0) { return addNode((void*)data); }

	void setParent(FkNodeRef parent=0);
	void reparent(FkNodeRef parent=0);
	void detachChildren(bool preserveTransforms=false);

	void setLocal(const AffineMatrix& matrix);
	void setPosition(vec2 position);
	void setPosition(float x, float y) { setPosition(vec(x,y)); }
	void setAttitude(vec2 attitude);
	void setAttitude(float x, float y) { setAttitude(vec(x,y)); }
	void setRotation(float radians);
	void setScale(vec2 scale);
	void setScale(float x, float y) { setScale(vec(x,y)); }
	void setWorld(const AffineMatrix& matrix);
	void setUserData(void *userData);

	void apply(const AffineMatrix& matrix) { setLocal(matrix * local()); }

	FkContext* context() const;
	FkNodeRef parent() const;
	int level() const;
	const AffineMatrix& local() const;
	vec2 position() const;
	vec2 right() const;
	vec2 up() const;
	const AffineMatrix& world() const;
	const AffineMatrix* cachedTransform() const;
	void *userData() const;

	template<typename T>
	T *get() const { return (T*) userData(); }

	template<typename T>
	void set(T* aData) { setUserData((void*)aData); }

	void destroy();
};

template<typename T>
FkNodeRef FkTreeRef::addNode(T *data) { return addNode((void*)data); }

//------------------------------------------------------------------------------
// ITERATORS (redo c++11 style?)
//------------------------------------------------------------------------------

// all nodes, in DAG order
struct FkTreeIterator {
	FkNode* current;

	FkTreeIterator(const FkContext *context);
	bool finished() const { return current == 0; }
	void next();

	FkNodeRef curr() { return FkNodeRef(current); }
};

// just top-level root nodes
struct FkRootIterator {
	FkNode* current;
	
	FkRootIterator(const FkContext *context);
	bool finished() const { return current == 0; }
	void next();

	FkNodeRef curr() { return FkNodeRef(current); }
};

// just immediate child nodes
struct FkChildIterator {
	FkNode *current;

	FkChildIterator(const FkNode* parent);
	bool finished() const { return current == 0; }
	void next();

	FkNodeRef curr() { return FkNodeRef(current); }
};

// subtree DAG traversal
struct FkSubtreeIterator {
	const FkNode *parent;
	FkNode *current;
	
	FkSubtreeIterator(const FkNode *parent);
	bool finished() const { return current == 0; }
	void next();

	FkNodeRef curr() { return FkNodeRef(current); }
};

// all nodes, in inverse-DAG order
struct FkInvTreeIterator {
	FkNode *current;

	FkInvTreeIterator(const FkContext *context);
	bool finished() const { return current == 0; }
	void next();	

	FkNodeRef curr() { return FkNodeRef(current); }
};

// subtree, inverse-DAG order
struct FkInvSubtreeIterator {
	const FkNode *parent;
	FkNode *current;
	
	FkInvSubtreeIterator(const FkNode *parent);
	bool finished() const { return current == 0; }
	void next();

	FkNodeRef curr() { return FkNodeRef(current); }
};
