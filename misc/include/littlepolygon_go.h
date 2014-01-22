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

// GOs provide a simple way to specify level data in a portable/game-agnostic way without
// having to roll a new binary schema, kind of like who PNGs relieve us of having to
// roll some new image format or WAVs and MP3s free us from having to re-invent samples
// and streaming music.  This implementation is intended to be paired with a reference
// "content compiler" which simplifies specifying and loading components through a
// designer-friendly "template-specialization" heirarchy, similar to CSS.

// TL;DR - portable metadata.

// GIFT IDEAS:
//   - GO Layers?

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

	// Each context is backed by a display tree.  In principle this should actually
	// be a component, but it's so common it might make sense to model it this way.
	FkTreeRef displayTree();

	// Game objects have serializable IDs
	GameObjectRef find(GameObjectID gid);
	GameObjectRef find(GoName name);

	// Explicit IDs are useful for deserialization and network syncronization.  Passing
	// 0 will just generate a new unique ID.  IDs may conflict, because they encode
	// storage, which will cause this method to fail.  Passing 1-N to a fresh context
	// is guarenteed to succeed.
	GameObjectRef addObject(GoName name="", GameObjectID explicitId=0);

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

	// These methods are forwarded to components, and their response is based
	// on runtime virtual-function dispatch.  They're required largely to support
	// common game idioms: object pools and mode-based activation of content.
	int enable();
	int disable();
	int sendMessage(int msg, const void *args=0);

	// GOs typically only have one component of a given type; however in the event
	// that they have more than one, you can find all instances by iterating over
	// them.  Otherwise, these methods are common enough to have first-class
	// status.
	GoComponentRef addComponent(GoComponentType *type, const void *args=0);
	GoComponentRef getComponent(GoComponentType *type);

	// Nuking a game object also destroys all it's children, components, and 
	// logical properties.
	void destroy();
};

//------------------------------------------------------------------------------
// COMPONENT TYPES
//------------------------------------------------------------------------------

class GoComponentType {
public:

	// Invoked when a component is added and before it's reference is publicized
	// to code.  Due to order-of-initialization being generally undefined, you should
	// not typically depend on sibling components here.
	// Backing-store for components is implemented using the userData handle here
	// as a Component->Implementation mapping.
	// In principle, component backing store should work *without the component
	// interface*.  This is just metadata, not an intrinsic part of the functionality
	// of the underlying system.  Our role is as a matchmaker, not a man-in-the-middle.
	virtual int init(GoComponentRef component, const void *args=0) = 0;

	// Components are disabled by default and must be explicitly enabled here.  For 
	// objects which don't support logicaly disabling, this is where they should
	// actually be instantiated then.  Calls to enable() and disable() can be interleaved,
	// but you won't get two enable() or disable() calls in a row.
	// During deserialization, all components will be init()'d before any are enabled,
	// so this is a good point for any initialization which depends on siblings.
	virtual int enable(GoComponentRef component) = 0;

	// Messages are the general mechanism for dispatching events which have an 
	// "inverted flow of control" or "observer" characteristic.  I haven't thought
	// about this too much, yet.  More to come.
	virtual int message(GoComponentRef component, int messageId, const void *args=0) { return 0; }

	// Disable the object but don't release it's resources.
	virtual int disable(GoComponentRef component) = 0;

	// Actually release resources - this GameObject is about to be nuked.
	virtual int release(GoComponentRef component) = 0;

	// Property getter/setting bindings.  The basic idiom I'm imagining is that
	// systems switch on the keyHash.
	virtual int getProperty(GoPropertyRef property) = 0;
	virtual int setProperty(GoPropertyRef property) = 0;
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
// This schema maps very cleanly into key-value stores and nosql databases.

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
	uint32_t keyHash() const;
	GoPropertyType type() const;

	int intValue() const;
	float floatValue() const;
	GoString stringValue() const;
	GameObjectRef refValue() const;

	void destroy();

};

//------------------------------------------------------------------------------
// ITERATORS
// These are the basic work-horses of in-game editors, allowing for databse-like
// "discovery" of object, components, and properties.  In principle, this should
// be all that's required to build a display-tree or inspector view.
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

// Iterate over all properties of a given component
struct GoPropertyIterator {
	GoPropertyRef current;

	GoPropertyIterator(GoComponentRef component);
	bool finished() const { return !current; }
	void next();
};


