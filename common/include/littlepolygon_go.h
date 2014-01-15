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

// Generic GameObject (GO) system for entity-component style assets.  The basic idea
// is to model content as a database of GOs, each of which is assembled (by data) out of
// a finite set of built-in (or scripted) components.  This contrasts with a typical
// object-oriented architecture which makes different trade-ffs by utilizing inheritance.

// TL;DR - very simple, portable metadata.

// No other systems depend on this module, however a utils package is available with
// reference bindings for other little polygon systems (fk, sprites) which you can use
// directly or reference.

struct GoContext;
struct GameObject;
struct GoComponent;
class GameObjectRef;
class GoComponentRef;
typedef Bitset<1024> GoSet;
typedef uint32_t GameObjectID;

//------------------------------------------------------------------------------
// COMPONENT SYSTEM ADAPTOR INTERFACE
//------------------------------------------------------------------------------

// In principle, component systems should no know about the GO database at all,
// but around "bound" by a third-party implementing this database.  We want to
// be able to use Go only when it's helpful, but omit it when it's not.

// Component Type ID 0 is "undefined"
// Component Type IDs 1-N are valid
typedef uint32_t ComponentTypeID;

#define GOSTATUS_OK 0
#if DEBUG
#define GOSTATUS_CHECK(_expr) { int __result__=_expr; if(__result__) return __result__; }
#else
#define GOSTATUS_CHECK(_expr)
#endif

class GoSystem {
private:
	ComponentTypeID type;

public:

	GoSystem(ComponentTypeID aType) : type(aType) { ASSERT(type != 0); }

	// The type of component that this interface handles.  One logical system can
	// handle multiple component types by implementing multiple GoSystem adaptors
	// with a shared backend, however multiple systems cannot handle the same
	// component type ID.
	ComponentTypeID comType() const { return type; }

	// Called when a component is added, before the handle is exposed
	// for other components and scripts to query.
	virtual int onInit(GoComponent *component, const void *args) = 0;

	// Called when a gameobject is enabled.  In the normal serialization
	// flow a gameobject is full initialized with components before it's
	// enabled, so this is where it's OK to look up sibling components.
	virtual int onEnable(GoComponent *component) = 0;

	// Optional handler for custom messages.  These are for events which are "dispatched"
	// to game objects but aren't necessarily aware of the receivers.
	virtual int onMessage(GoComponent *component, int messageId, const void *args) { return 0; }

	// Called when a gameobject is disabled but not destroyed.  Useful for
	// implementing common idioms like object pools and state machines.  May
	// be interleaved with calls to onEnable.
	virtual int onDisable(GoComponent *component) = 0;

	// Called to destroy the object and release all resources associated
	// with it.
	virtual int onDestroy(GoComponent *component) = 0;

};

//------------------------------------------------------------------------------
// GO CONTEXT
//------------------------------------------------------------------------------

// You can create multiple contexts, e.g. for handling different rooms or 
// orthogonal views (e.g. HUD vs Scene), and synchronizing different remote
// network contexts.  Contexts can share systems.
GoContext *createGoContext(
	size_t numSystems, GoSystem **systemBuf, size_t totalComponentTypes=32,
	size_t goCapacity=1024, size_t componentCapacity=1024
);

void destroy(GoContext *context);

GoSystem *getSystem(const GoContext *context, ComponentTypeID cid);
GameObject *find(const GoContext *context, GameObjectID gid);
GameObject* find(const GoContext *context, const char *name);

//------------------------------------------------------------------------------
// GAME OBJECTS
//------------------------------------------------------------------------------

// Explicit IDs are useful for deserialization and network syncronization.  Passing
// 0 will just generate a new one.  IDs may conflict, which will cause this method to
// fail.  Passing 1-N to a fresh context should work OK.
GameObject* createGameObject(GoContext *context, const char* name=0, GameObjectID explicitId=0);

// Destroying a GO also nukes all the components that are logically attached to it.
void destroy(GameObject *go);

// Getters
GameObjectID goID(const GameObject *go);
GoContext *goContext(const GameObject *go);
bool goEnabled(const GameObject *go);
const char* goName(const GameObject *go);

// Methods
int enable(GameObject *go);
int disable(GameObject *go);
int sendMessage(GameObject *go, int messageId, const void *args=0);

//------------------------------------------------------------------------------
// GAME OBJECT COMPONENTS
//------------------------------------------------------------------------------

// Attach a component to the given game object, initialized with the given data.  CID 
// corresponds to the index of the component message handler that the context was
// intialized with.
GoComponent *addComponent(GameObject *go, ComponentTypeID cid, const void *data=0);
void destroy(GoComponent *component);


// lookup a component by type.  Returns the first match.  For an exhaustive search
// (in the event of multiple components of the same type), use the component iterator
GoComponent *getComponent(GameObject *go, ComponentTypeID cid);

// getters
ComponentTypeID comType(const GoComponent *component);
GameObject *comObject(const GoComponent *component);
void *comData(const GoComponent *component);

// setter
void setComData(GoComponent *component, void *data);

//------------------------------------------------------------------------------
// C++ Interface
//------------------------------------------------------------------------------

class GameObjectRef {
private:
	GameObject *go;

public:
	GameObjectRef() {}
	GameObjectRef(GameObject *aGo) : go(aGo) {}

	operator GameObject*() { return go; }
	operator bool() const { return go != 0; }

	GoContext *context() const { return goContext(go); }
	bool enabled() const { return goEnabled(go); }
	const char* name() const { return goName(go); }

	int enable() { return ::enable(go); }
	int disable() { return ::disable(go); }
	int sendMessage(int msg, const void *args) { return ::sendMessage(go, msg, args); }

	inline GoComponentRef addComponent(ComponentTypeID cid, const void *data=0);
	inline GoComponentRef getComponent(ComponentTypeID cid);

};

class GoComponentRef {
private:
	GoComponent *com;

public:
	GoComponentRef() {}
	GoComponentRef(GoComponent *aCom) : com(aCom) {}

	operator GoComponent*() { return com; }
	operator bool() const { return com != 0; }

	ComponentTypeID componentType() const { return comType(com); }
	GameObjectRef gameObject() const { return comObject(com); }

	template<typename T>
	T* data() { return (T*)comData(com); }

};

inline GoComponentRef GameObjectRef::addComponent(ComponentTypeID cid, const void *data) { 
	return ::addComponent(go, cid, data); 
}

inline GoComponentRef GameObjectRef::getComponent(ComponentTypeID cid) { 
	return ::getComponent(go, cid); 
}

//------------------------------------------------------------------------------
// ITERATORS
//------------------------------------------------------------------------------

struct GoIterator {
	GameObject *current;
	GoSet::iterator internal;

	GoIterator(GoContext *context);
	bool finished() const { return current == 0; }
	GameObjectRef ref() { return current; }
	
	void next();
};

struct GoComponentIterator {
	GoComponent *current;

	GoComponentIterator(GameObject *go);
	bool finished() const { return current == 0; }
	GoComponentRef ref() { return current; }

	void next();
};


