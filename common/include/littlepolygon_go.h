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

// No other systems depend on this module, however a utils package is available with
// reference bindings for other little polygon systems (fk, sprites) which you can use
// directly or reference.

struct GoContext;
struct GoComponent;
struct GameObject;
class GameObjectRef;
typedef Bitset<1024> GoSet;
typedef uint32_t GameObjectID;

#define GOSTATUS_OK 0
#if DEBUG
#define GOSTATUS_CHECK(_expr) { int __result__=_expr; if(__result__) return __result__; }
#else
#define GOSTATUS_CHECK(_expr)
#endif


struct GoComponent {
	// the game object we're attached to
	// (undefined until init)
	GameObject *go;

	// components are attached using an intrusive linked-list
	// (undefined until init)
	GoComponent *prev;
	GoComponent *next;

	// convenience getter
	inline GameObjectRef gameObject();

	// called after a component is attached to a GameObject
	virtual int init() = 0;

	// called when a gameObject is enabled, or immediately after
	// begin attached to an active gameObject.
	virtual int enable() = 0;

	// Generic message handler
	virtual int message(int messageId, const void *args) { return 0; }

	// called when a gameObject is disabled.
	virtual int disable() = 0;

	// called when a gameObject is destroyed.
	virtual int destroy() = 0;
};

//------------------------------------------------------------------------------
// GO CONTEXT
//------------------------------------------------------------------------------

// You can create multiple contexts, e.g. for handling different rooms or 
// orthogonal views (e.g. HUD vs Scene), and synchronizing different remote
// network contexts.  Contexts can share systems.
GoContext *createGoContext(FkContext *fkContext, size_t capacity);
void destroy(GoContext *context);

GameObject *find(GoContext *context, GameObjectID gid);
GameObject* find(GoContext *context, const char *name);

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
GoContext *goContext(const GameObject *go);
bool goEnabled(const GameObject *go);
const char* goName(const GameObject *go);
FkNode* goNode(const GameObject *go);

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
void addComponent(GameObject *go, GoComponent *component);
GoComponent *firstComponent(GameObject *go);
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
	operator bool() const { return go != 0; }

	GoContext *context() const { return goContext(go); }
	bool enabled() const { return goEnabled(go); }
	const char* name() const { return goName(go); }

	int enable() { return ::enable(go); }
	int disable() { return ::disable(go); }
	int sendMessage(int msg, const void *args) { return ::sendMessage(go, msg, args); }

	inline void addComponent(GoComponent *component) { ::addComponent(go, component); }

	FkNodeRef node() { return goNode(go); }

	// lookup a component by type.  Returns the first match.  For an exhaustive search
	// (in the event of multiple components of the same type), use the component iterator
	template<typename T>
	T *getComponent() {
		for(GoComponent *p=firstComponent(go); p; p=p->next) {
			T *result = dynamic_cast<T*>(p);
			if (result) { 
				return result; 
			}
		}
		return 0;
	}

	void destroy() { ::destroy(go); }
};

inline GameObjectRef GoComponent::gameObject() { return go; }

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
