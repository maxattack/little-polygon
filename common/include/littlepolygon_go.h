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

// Generic GameObject (GO) system for entity-component style assets.  The basic idea
// is to model content as a database of GOs, each of which is assembled (by data) out of
// a finite set of built-in (or scripted) components.  This contrasts with a typical
// object-oriented architecture which makes different trade-ffs by utilizing inheritance.

// TL;DR - very simple, portable metadata.

struct GoContext;
struct GameObject;
struct GoComponent;
class GameObjectRef;
class GoComponentRef;
typedef uint32_t GameObjectID;
typedef uint32_t ComponentTypeID;

// Interface for systems which manage the implementation of go components
// and implement basic system messages.

class GoComponentSystem {
public:

	// Called when a component is added, before the handle is exposed
	// for other components and scripts to query.
	virtual int onInit(GoContext *context, GoComponent *component, const void *args) = 0;

	// Called when a gameobject is enabled.  In the normal serialization
	// flow a gameobject is full initialized with components before it's
	// enabled, so this is where it's OK to look up sibling components.
	virtual int onEnable(GoContext *context, GoComponent *component) = 0;

	// Optional handler for custom messages.  These are for events which are "dispatched"
	// to game objects but aren't necessarily aware of the receivers.
	virtual int onMessage(GoContext *context, GoComponent *component, int messageId, const void *args) { return 0; }

	// Called when a gameobject is disabled but not destroyed.  Useful for
	// implementing common idioms like object pools and state machines.  May
	// be interleaved with calls to onEnable.
	virtual int onDisable(GoContext *context, GoComponent *component) = 0;

	// Called to destroy the object and release all resources associated
	// with it.
	virtual int onDestroy(GoContext *context, GoComponent *component) = 0;

};

// Initialize a new GO context.  These are in principle serializable to ease the 
// implementation of "in-game editing" as well as syncable across networks for 
// "remote contexts" like in MMOs. Capacity is needed because the whole database
// is "block-allocated."
GoContext *createGoContext(
	size_t numSystems, GoComponentSystem **systemBuf, 
	size_t goCapacity=1024, size_t componentCapacity=1024
);
void destroy(GoContext *context);

// Retrieve the context for the given component type - basically a pointer to it's
// batching system.
GoComponentSystem *getSystem(GoContext *context, ComponentTypeID cid);

// Explicit IDs are useful for deserialization and network syncronization.  Passing
// 0 will just generate a new one.  IDs may conflict, which will cause this method to
// fail.  Passing 1-N to a fresh context should work OK.
GameObject* createGameObject(GoContext *context, const char* name=0, GameObjectID explicitId=0);

GameObjectID goID(const GameObject *go);
GameObject *getGameObject(const GoContext *context, GameObjectID gid);

// Destroying a GO also nukes all the components that are logically attached to it.
void destroy(GameObject *go);

// Lookup a game object by name.  In the case of duplicate-names, simply returns the
// first one it finds.
GameObject* find(GoContext *context, const char *name);

// Getters
GoContext *goContext(const GameObject *go);
bool goEnabled(const GameObject *go);
const char* goName(const GameObject *go);

void enable(GameObject *go);
void disable(GameObject *go);
void sendMessage(GameObject *go, int messageId, const void *args=0);

// Attach a component to the given game object, initialized with the given data.  CID 
// corresponds to the index of the component message handler that the context was
// intialized with.
GoComponent *addComponent(GameObject *go, ComponentTypeID cid, const void *data=0);

// lookup a component by type.  Returns the first match.  For an exhaustive search
// (in the event of multiple components of the same type), use the component iterator
GoComponent *getComponent(GameObject *go, ComponentTypeID cid);

ComponentTypeID comType(const GoComponent *component);
GameObject *comObject(const GoComponent *component);
void *comData(const GoComponent *component);

// Remove a component, destroying it completely.
void destroy(GoComponent *component);

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
	operator const GameObject*() { return go; }

	GoContext *context() const { return goContext(go); }
	bool enabled() const { return goEnabled(go); }
	const char* name() const { return goName(go); }

	void enable() { ::enable(go); }
	void disable() { ::disable(go); }
	void sendMessage(int msg, const void *args) { ::sendMessage(go, msg, args); }

	inline GoComponentRef addComponent(ComponentTypeID cid, const void *data=0);
	inline GoComponentRef getComponent(ComponentTypeID cid);

};

class GoComponentRef {
private:
	GoComponent *com;

public:
	GoComponentRef() {}
	GoComponentRef(GoComponent *aCom) : com(aCom) {}

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


