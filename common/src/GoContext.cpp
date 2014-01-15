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

#include "littlepolygon_go.h"

//------------------------------------------------------------------------------
// RECORDS
//------------------------------------------------------------------------------

// Collections are maintained using an intrusive doubly-linked list between
// pre-allocated slots - no dynamic memory after init.

struct GoComponent {
	ComponentTypeID type;
	GameObject *go;
	void *userData;
	GoComponent *prev;
	GoComponent *next;
};

struct GameObject {
	uint32_t nameHash;
	const char *name;
	GoComponent *coFirst;
	GoContext *context;
};

struct GoContext {
	size_t goCapacity;
	size_t goCount;
	size_t coCapacity;
	size_t coCount;
	size_t coTypeCount;

	GoSet goAllocationMask;
	GoSet goEnabledMask;

	GoComponent *coFirstFree;

	GoSystem *headSystem;

	GoSystem **coTypeBuf() { return &headSystem; }
	GameObject *goBuf() const { return (GameObject*)((&headSystem) + coTypeCount); }
	GoComponent *coBuf() const { return (GoComponent*)(goBuf() + goCapacity);}

	GoSystem *getSystem(ComponentTypeID cid) const {
		ASSERT(cid > 0);
		ASSERT(cid <= coTypeCount);
		return *(&headSystem + (cid-1));
	}

};

//------------------------------------------------------------------------------
// HELPERS
//------------------------------------------------------------------------------

#define GO_INDEX(go) ((0xffff & go)-1)

// fnv-1a
static uint32_t hash(const char* name) {
    uint32_t hval = 0x811c9dc5;
    while(*name) {
        hval ^= (*name);
        hval *= 0x01000193;
        ++name;
    }
    return hval;
}

//------------------------------------------------------------------------------
// GO CONTEXT
//------------------------------------------------------------------------------

GoContext *createGoContext(
	size_t numSystems, GoSystem **systemBuf, 
	size_t coTypeCount,
	size_t goCapacity, 
	size_t coCapacity
) {
	ASSERT(goCapacity <= 1024);

	// block allocate the memory
	GoContext *context = (GoContext*) LITTLE_POLYGON_MALLOC(
		sizeof(GoContext) + 
		(coTypeCount-1) * sizeof(GoSystem*) +
		goCapacity * sizeof(GameObject) + 
		coCapacity * sizeof(GoComponent)
	);

	// initialize fields
	context->goCapacity = goCapacity;
	context->goCount = 0;
	context->coCapacity = coCapacity;
	context->coCount = 0;
	context->coTypeCount = coTypeCount;
	context->coFirstFree = context->coBuf();
	context->goAllocationMask =GoSet();
	context->goEnabledMask = GoSet();

	// initialize typeToSystem map
	memset(context->coTypeBuf(), 0, sizeof(GoSystem*) * coTypeCount);
	for(size_t i=0; i<coTypeCount; ++i) {
		for(auto p=systemBuf[0]; p!=systemBuf[numSystems]; ++p) {
			if (p->comType() == i+1) {
				context->coTypeBuf()[i] = p;
				break;
			}
		}
	}
	
	// initialize free-slot linked lists
	auto coEnd = context->coBuf() + coCapacity;
	for(auto c=context->coBuf(); c!=coEnd; ++c) {
		c->prev = c == context->coBuf() ? 0 : c-1;
		c->next = c == coEnd-1          ? 0 : c+1;
	}
	
	return context;
}

void destroy(GoContext *context) {
	
	// tear-down all the game objects
	unsigned index;
	while(context->goAllocationMask.findFirst(index)) {
		destroy(context->goBuf() + index);
	}
	
	// de-alloc the memory
	LITTLE_POLYGON_FREE(context);
}

GoSystem *getSystem(const GoContext *context, ComponentTypeID cid) {
	return context->getSystem(cid);
}

GameObject *find(const GoContext *context, GameObjectID gid) {
	if (gid == 0 || gid >= context->goCapacity) {
		return 0;
	} else {
		return context->goBuf() + (gid-1);
	}
}

GameObject *find(const GoContext *context, const char *name) {
	// pretty dumb O(N) lookup - not meant for mission-critical code
	auto fnv = hash(name);
	unsigned idx;
	for(auto i=context->goAllocationMask.listBits(); i.next(idx);) {
		GameObject *go = context->goBuf() + idx;
		if (go->nameHash == fnv && strcmp(name, go->name) == 0) {
			return go;
		}
	}
	return 0;
}



//------------------------------------------------------------------------------
// GAME OBJECTS
//------------------------------------------------------------------------------

GameObject* createGameObject(GoContext *context, const char* name, GameObjectID explicitId) {
	
	// check capacity
	if (context->goCount == context->goCapacity) {
		return 0;
	}

	// determine slot
	unsigned index;
	if (explicitId) {
		// verify that the slot is free
		index = GO_INDEX(explicitId);
		ASSERT(index < context->goCapacity);
		if (context->goAllocationMask[index]) {
			return 0;
		}
	} else {
		// generate a new ID
		if (!(~context->goAllocationMask).findFirst(index)) {
			return 0;
		}
	}
	context->goAllocationMask.mark(index);
	GameObject *go = context->goBuf() + index;

	// initialize fields
	go->nameHash = hash(name);
	go->name = name;
	go->coFirst = 0;
	go->context = context;

	// finalize bookkeeping and return
	++context->goCount;
	return go;
}

void destroy(GameObject *go) {
		
	// lookup slot
	ASSERT(go->context->goAllocationMask[go - go->context->goBuf()]);

	// destroy components
	while(go->coFirst) {
		destroy(go->coFirst);
	}

	// udpate fields
	go->context->goAllocationMask.clear(go - go->context->goBuf());
	--go->context->goCount;
}

GameObjectID goID(const GameObject *go) {
	// todo fingerprint IDs?
	if (go < go->context->goBuf() || go > go->context->goBuf() + go->context->goCapacity) {
		return 0;
	} else {
		return 1 + GameObjectID(go - go->context->goBuf());
	}
}

GoContext *goContext(const GameObject *go) {
	return go->context;
}

bool goEnabled(const GameObject *go) {
	return go->context->goEnabledMask[go - go->context->goBuf()];
}

const char* goName(const GameObject *go) {
	return go->name;
}

int enable(GameObject *go) {
	if (!goEnabled(go)) {
		for(auto c=go->coFirst; c; c=c->next) {
			auto sys = go->context->getSystem(c->type);
			ASSERT(sys);
			GOSTATUS_CHECK( sys->onEnable(c) );
		}
		go->context->goEnabledMask.mark(go - go->context->goBuf());
	}
	return GOSTATUS_OK;
}

int disable(GameObject *go) {
	if (goEnabled(go)) {
		for(auto c=go->coFirst; c; c=c->next) {
			auto sys = go->context->getSystem(c->type);
			ASSERT(sys);
			GOSTATUS_CHECK( sys->onDisable(c) );
		}
		go->context->goEnabledMask.clear(go - go->context->goBuf());
	}
	return GOSTATUS_OK;
}

int sendMessage(GameObject *go, int messageId, const void *args) {
	for(auto c=go->coFirst; c; c=c->next) {
		auto sys = go->context->getSystem(c->type);
		ASSERT(sys);
		GOSTATUS_CHECK( sys->onMessage(c, messageId, args) );
	}
	return GOSTATUS_OK;
}

//------------------------------------------------------------------------------
// COMPONENTS
//------------------------------------------------------------------------------

GoComponent *addComponent(GameObject *go, ComponentTypeID cid, const void *data) {
	ASSERT(go->context->coCount < go->context->coCapacity);
	auto sys = go->context->getSystem(cid);
	if (!sys) { return 0; }

	// pop a slot from the component free list
	auto result = go->context->coFirstFree;
	go->context->coFirstFree = go->context->coFirstFree->next;
	if (result->next) { result->next->prev = result->prev; }
	if (result->prev) { result->prev->next = result->next; }

	// prepend to go's component list
	result->prev = 0;
	result->next = go->coFirst;
	if (go->coFirst) { go->coFirst->prev = result; }
	go->coFirst = result;

	// init fields
	result->type = cid;
	result->go = go;
	result->userData = 0;

	// logically initialize
	sys->onInit(result, data);
	if (goEnabled(go)) {
		sys->onEnable(result);
	}

	// update bookkeeping and return
	++go->context->coCount;
	return result;
}

GoComponent *getComponent(GameObject* go, ComponentTypeID cid) {
	for(auto c=go->coFirst; c; c=c->next) {
		if (c->type == cid) {
			return c;
		}
	}
	return 0;
}

ComponentTypeID comType(const GoComponent *component) {
	return component->type;
}

GameObject *comObject(const GoComponent *component) {
	return component->go;
}

void *comData(const GoComponent *component) {
	return component->userData;
}

void setComData(GoComponent *component, void *data) {
	component->userData = data;
}

void destroy(GoComponent *component) {
	auto sys = component->go->context->getSystem(component->type);
	ASSERT(sys);

	// logically destroy
	sys->onDestroy(component);

	// remove from GO list
	if (component == component->go->coFirst) { component->go->coFirst = component->next; }
	if (component->next) { component->next->prev = component->prev; }
	if (component->prev) { component->prev->next = component->next; }

	// add to free list
	component->prev = 0;
	component->next = component->go->context->coFirstFree;
	if (component->go->context->coFirstFree) { component->go->context->coFirstFree->prev = component; }
	component->go->context->coFirstFree = component;
	--component->go->context->coCount;
}

//------------------------------------------------------------------------------
// ITERATORS
//------------------------------------------------------------------------------

GoIterator::GoIterator(GoContext *context) : 
current(0),
internal(context->goAllocationMask.listBits()) {
	unsigned index;
	if (internal.next(index)) {
		current = context->goBuf() + index;
	}
}

void GoIterator::next() { 
	unsigned index;
	current = internal.next(index) ? current->context->goBuf() + index : 0;
}

GoComponentIterator::GoComponentIterator(GameObject *go) {
	current = go->coFirst;
}

void GoComponentIterator::next() { 
	current = current->next;
}
