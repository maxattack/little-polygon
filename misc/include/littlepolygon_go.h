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

// Generic GameObject (GO) system for entity-component style assets.  The basic idea
// is to model content as a database of GOs, each of which is assembled (by data) out of
// a finite set of built-in (or scripted) components.  This contrasts with a typical
// object-oriented architecture which makes different trade-ffs by utilizing inheritance.

// TL;DR - very simple, portable metadata.

// GIFT IDEAS:
//   - generic uint32_t mask for each GO?

//------------------------------------------------------------------------------
// TYPES
//------------------------------------------------------------------------------

#ifndef GO_CAPACITY
#define GO_CAPACITY 1024
#endif

struct GoContext;
struct GoComponent;
struct GameObject;
class GameObjectRef;
class GoComponentRef;
typedef Bitset<GO_CAPACITY> GoSet;
typedef uint32_t GameObjectID;

class GoComponentType {
public:
	virtual int init(GoComponent* component, const void *args=0) = 0;
	virtual int enable(GoComponent* component) = 0;
	virtual int message(GoComponent* component, int messageId, const void *args=0) { return 0; }
	virtual int disable(GoComponent* component) = 0;
	virtual int release(GoComponent* component) = 0;
};

//------------------------------------------------------------------------------
// HELPERS
//------------------------------------------------------------------------------

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
GoContext *createGoContext(FkContext *fkContext, size_t coCapacity=4096);
void destroy(GoContext *context);

FkContext *fkContext(GoContext *context);

GameObject *find(GoContext *context, GameObjectID gid);
GameObject* find(GoContext *context, const char *name);

//------------------------------------------------------------------------------
// GAME OBJECTS
//------------------------------------------------------------------------------

// Explicit IDs are useful for deserialization and network syncronization.  Passing
// 0 will just generate a new one.  IDs may conflict, which will cause this method to
// fail.  Passing 1-N to a fresh context should work OK.
GameObject* addGameObject(GoContext *context, const char* name="", GameObjectID explicitId=0);
void destroy(GameObject *go);

// Getters
GoContext *goContext(const GameObject *go);
bool goEnabled(const GameObject *go);
const char* goName(const GameObject *go);
FkNode* goNode(const GameObject *go);
GoComponent* goComponent(GameObject *go, GoComponentType* type);
GameObject *goFromNode(FkNode *node);

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
GoComponent* addComponent(GameObject *go, GoComponentType* type, const void* args=0);
void destroy(GoComponent *component);

// Methods
GoComponentType* coType(GoComponent *component);
void* coHandle(GoComponent *component);
GameObject* coObject(GoComponent *component);

void setHandle(GoComponent *component, void *handle);

//------------------------------------------------------------------------------
// ITERATORS
//------------------------------------------------------------------------------

// Iterates over all game objects
struct GoIterator {
	GameObject *current;
	GoSet::iterator internal;

	GoIterator(GoContext *context);
	bool finished() const { return current == 0; }	
	void next();
};

// Iterates over all components of a given game object
struct GoComponentIterator {
	GoComponent *current;

	GoComponentIterator(GameObject *go);
	bool finished() const { return current == 0; }
	void next();
};


//------------------------------------------------------------------------------
// More Idomatic C++ Interface
//------------------------------------------------------------------------------

class GameObjectRef {
private:
	GameObject *go;

public:
	GameObjectRef() {}
	GameObjectRef(GameObject *aGo) : go(aGo) {}

	operator GameObject*&() { return go; }
	operator bool() const { return go != 0; }

	GoContext *context() const { return goContext(go); }
	bool enabled() const { return goEnabled(go); }
	const char* name() const { return goName(go); }

	int enable() { return ::enable(go); }
	int disable() { return ::disable(go); }
	int sendMessage(int msg, const void *args) { return ::sendMessage(go, msg, args); }

	inline GoComponentRef addComponent(GoComponentType *type);

	template<typename T>
	inline GoComponentRef addComponent(GoComponentType *type, T* args);

	inline GoComponentRef getComponent(GoComponentType *type);

	FkNodeRef node() { return goNode(go); }

	void destroy() { ::destroy(go); go = 0; }
};

class GoComponentRef {
private:
	GoComponent *component;

public:
	GoComponentRef() {}
	GoComponentRef(GoComponent *aComponent) : component(aComponent) {}

	operator GoComponent*&() { return component; }
	operator bool() const { return component != 0; }

	GoComponentType* type() { return coType(component); }
	GameObjectRef gameObject() { return coObject(component); }

	template<typename T>
	T* get() { return (T*)coHandle(component); }

	template<typename T>
	void set(T* handle) { setHandle(component, (void*)handle); }

	void destroy() { ::destroy(component); component=0; }
};

inline GoComponentRef GameObjectRef::addComponent(GoComponentType *type) { return ::addComponent(go, type); }
inline GoComponentRef GameObjectRef::getComponent(GoComponentType *type) { return ::goComponent(go, type); }

template<typename T>
inline GoComponentRef GameObjectRef::addComponent(GoComponentType *type, T* args) {
	return ::addComponent(go, type, (const void*) args);
}

