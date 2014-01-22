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
#include "littlepolygon_bitset.h"
#include "littlepolygon_fk.h"
#include <string>

// Generic GameObject (GO) system for entity-component style assets.  The basic idea
// is to model content as a database of GOs, each of which is assembled (by data) out of
// a finite set of built-in (or scripted) components.  This contrasts with a typical
// object-oriented architecture which makes different trade-ffs by utilizing inheritance.

// Each component has dynamic properties which can be interrogated by other systems,
// e.g. an in-game editor.

// TL;DR - very simple, portable metadata.

// GIFT IDEAS:
//   - generic uint32_t mask for each GO?

//------------------------------------------------------------------------------
// TYPES
//------------------------------------------------------------------------------

struct GoContext;
struct GameObject;
struct GoComponent;
struct GoProperty;
class GoContextRef;
class GameObjectRef;
class GoComponentRef;
class GoPropertyRef;

typedef Bitset<GO_CAPACITY> GoSet;
typedef uint32_t GameObjectID;
typedef std::string GoName;
typedef std::string GoString;

#define GOSTATUS_OK 0
#if DEBUG
#define GOSTATUS_CHECK(_expr) { int __result__=_expr; if(__result__) return __result__; }
#else
#define GOSTATUS_CHECK(_expr)
#endif

//------------------------------------------------------------------------------
// GO CONTEXT
//------------------------------------------------------------------------------

// You can create multiple contexts, e.g. for handling different rooms or 
// orthogonal views (e.g. HUD vs Scene), and synchronizing different remote
// network contexts.  Contexts can share systems.
GoContext *createGoContext(
	FkTreeRef fkTree, 
	size_t goCapacity=1024, 
	size_t coCapacity=4096
);

class GoContextRef {
private:
	GoContext *context;

public:
	GoContextRef() {}
	GoContextRef(GoContext *aContext) : context(aContext) {}

	operator GoContext*() { return context; }
	operator bool() const { return context; }

	void destroy();

	FkTreeRef displayTree();

	GameObjectRef find(GameObjectID gid);
	GameObjectRef find(GoName name);

	// Explicit IDs are useful for deserialization and network syncronization.  Passing
	// 0 will just generate a new one.  IDs may conflict, which will cause this method to
	// fail.  Passing 1-N to a fresh context should work OK.
	GameObjectRef addObject(GoName="", GameObjectID explicitId=0);

};

//------------------------------------------------------------------------------
// GAME OBJECTS
//------------------------------------------------------------------------------

class GameObjectRef {
private:
	GameObject *go;

public:
	GameObjectRef() {}
	GameObjectRef(GameObject *aGo) : go(aGo) {}

	operator GameObject*() { return go; }
	operator bool() const { return go != 0; }

	GoContextRef context() const;
	bool enabled() const;
	GoName name() const;
	FkNodeRef node();

	int enable();
	int disable();
	int sendMessage(int msg, const void *args=0);

	GoComponentRef addComponent(GoComponentType *type, const void *args=0);
	GoComponentRef getComponent(GoComponentType *type);

	void destroy();
};

//------------------------------------------------------------------------------
// COMPONENT TYPES
//------------------------------------------------------------------------------

class GoComponentType {
public:
	virtual int init(GoComponentRef component, const void *args=0) = 0;
	virtual int enable(GoComponentRef component) = 0;
	virtual int message(GoComponentRef component, int messageId, const void *args=0) { return 0; }
	virtual int disable(GoComponentRef component) = 0;
	virtual int release(GoComponentRef component) = 0;
};

//------------------------------------------------------------------------------
// COMPONENT INSTANCES
//------------------------------------------------------------------------------

class GoComponentRef {
private:
	GoComponent *component;

public:
	GoComponentRef() {}
	GoComponentRef(GoComponent *aComponent) : component(aComponent) {}

	operator GoComponent*() { return component; }
	operator bool() const { return component != 0; }

	GoComponentType* type();
	GameObjectRef gameObject();
	void *userData();

	void setUserData(void *data);

	template<typename T>
	T* get() { return (T*)userData(); }

	template<typename T>
	void set(T* handle) { setUserData((void*)handle); }

	GoPropertyRef getProperty(GoName key);

	void destroy();
};

//------------------------------------------------------------------------------
// PROPERTIES
//------------------------------------------------------------------------------

enum GoPropertyType {
	PROPERTY_UNDEFINED,
	PROPERTY_INT,
	PROPERTY_FLOAT,
	PROPERTY_NAME,
	PROPERTY_STRING,
	PROPERTY_GO_REFERENCE
};

// Compound properties are created using formatted keys. For dictionary
// or struct-like properties, use dot-syntax, e.g.:
//
//   position.x = 314
//   position.y = 271
//   position.z = 0
//
// For arrays, use backet [] syntax, e.g.:
//
//   list[0] = 1
//   list[1] = 1
//   list[2] = 2
//   list[3] = 3
//   list[4] = 5
// 
// These syntaxes can be combined to produces lists-of-structs and 
// structs-of-lists, e.g.:
// 
//   positions[0].x = 1
//   positions[0].y = 2
//   positions[1].x = 0
//   positions[1].x = 42
//
// This schema maps very cleanly into key-value stores and nosql databases

class GoPropertyRef {
private:
	GoProperty *prop;

public:
	GoPropertyRef() {}
	GoPropertyRef(GoProperty *aProp) : prop(aProp) {}

	operator GoProperty*() { return prop; }
	operator bool() const { return prop; }

	GoComponentRef component();

	GoName key() const;
	GoPropertyType type() const;

	int intValue() const;
	float floatValue() const;
	GoName nameValue() const;
	GoString stringValue() const;
	GameObjectRef refValue() const;

	void destroy();

};

//------------------------------------------------------------------------------
// ITERATORS
//------------------------------------------------------------------------------

// Iterates over all game objects
struct GoIterator {
	GameObjectRef current;
	GoSet::iterator internal;

	GoIterator(GoContextRef context);
	bool finished() const { return !current; }	
	void next();
};

// Iterates over all components of a given game object
struct GoComponentIterator {
	GoComponentRef current;

	GoComponentIterator(GameObjectRef go);
	bool finished() const { return !current; }
	void next();
};

struct GoPropertyIterator {
	GoPropertyRef current;

	GoPropertyIterator(GoComponentRef component);
	bool finished() const { return !current; }
	void next();
};


