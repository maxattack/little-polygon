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

struct GameObject {
	uint32_t nameHash;
	const char *name;
	GoComponent *coFirst;
	FkNode *node;
	GoContext *context;
};

struct GoContext {
	size_t goCapacity;
	size_t goCount;
	FkContext *fkContext;
	GoSet goAllocationMask;
	GoSet goEnabledMask;

	GameObject headGo;

	GameObject *goBuf() { return &headGo; }
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

GoContext *createGoContext(FkContext *fkContext, size_t goCapacity) {
	ASSERT(goCapacity <= 1024);

	// block allocate the memory
	GoContext *context = (GoContext*) LITTLE_POLYGON_MALLOC(
		sizeof(GoContext) + (goCapacity-1) * sizeof(GameObject)
	);

	// initialize fields
	context->goCapacity = goCapacity;
	context->goCount = 0;
	context->fkContext = fkContext;	
	context->goAllocationMask =GoSet();
	context->goEnabledMask = GoSet();
	
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

GameObject *find(GoContext *context, GameObjectID gid) {
	if (gid == 0 || gid >= context->goCapacity) {
		return 0;
	} else {
		return context->goBuf() + (gid-1);
	}
}

GameObject *find(GoContext *context, const char *name) {
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
	go->node = createNode(context->fkContext, 0, go);

	// finalize bookkeeping and return
	++context->goCount;
	return go;
}

void destroy(GameObject *go) {
		
	// lookup slot
	ASSERT(go->context->goAllocationMask[go - go->context->goBuf()]);

	// destroy children
	auto iter = FkChildIterator(go->node);
	while(!iter.finished()) {
		auto child = iter.ref().data<GameObject>();
		iter.next();
		destroy(child);
	}


	// destroy components
	while(go->coFirst) {
		destroy(go->coFirst);
	}

	// destroy node
	destroy( go->node );

	// udpate fields
	go->context->goAllocationMask.clear(go - go->context->goBuf());
	go->context->goEnabledMask.clear(go - go->context->goBuf());
	--go->context->goCount;
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

FkNode* goNode(const GameObject *go) {
	return go->node;
}

int enable(GameObject *go) {
	if (!goEnabled(go)) {
		for(auto c=go->coFirst; c; c=c->next) {
			GOSTATUS_CHECK(c->enable());
		}
		for(FkChildIterator i(go->node); !i.finished(); i.next()) {
			GOSTATUS_CHECK( enable(i.ref().data<GameObject>()) );
		}
		go->context->goEnabledMask.mark(go - go->context->goBuf());
	}

	return GOSTATUS_OK;
}

int disable(GameObject *go) {
	if (goEnabled(go)) {
		for(auto c=go->coFirst; c; c=c->next) {
			GOSTATUS_CHECK(c->disable());
		}
		for(FkChildIterator i(go->node); !i.finished(); i.next()) {
			GOSTATUS_CHECK( disable(i.ref().data<GameObject>()) );
		}
		go->context->goEnabledMask.clear(go - go->context->goBuf());
	}
	return GOSTATUS_OK;
}

int sendMessage(GameObject *go, int messageId, const void *args) {
	for(auto c=go->coFirst; c; c=c->next) {
		GOSTATUS_CHECK(c->message(messageId, args));
	}
	for(FkChildIterator i(go->node); !i.finished(); i.next()) {
		GOSTATUS_CHECK( sendMessage(i.ref().data<GameObject>(), messageId, args) );
	}
	return GOSTATUS_OK;
}

//------------------------------------------------------------------------------
// COMPONENTS
//------------------------------------------------------------------------------

void addComponent(GameObject *go, GoComponent *component) {

	// we apply a heuristic here - the first component that's added will always
	// be at the head of the list.  This is because it's typically looked up 
	// frequently - it's most commonly a transform component - so we make sure it
	// doesn't get pushed to the end of the list, causing all lookups on it to be
	// worst-case.

	if (go->coFirst) {
		component->next = go->coFirst->next;
		if (component->next) { component->next->prev = component; }
		go->coFirst->next = component;
		component->prev = go->coFirst;
	} else {
		component->prev = 0;
		component->next = 0;
		go->coFirst = component;
	}

	// init fields
	component->go = go;
	component->init();
	if (goEnabled(go)) {
		component->enable();
	}
}

void destroy(GoComponent *component) {
	// logically destroy
	component->destroy();

	// remove from GO list
	if (component == component->go->coFirst) { component->go->coFirst = component->next; }
	if (component->next) { component->next->prev = component->prev; }
	if (component->prev) { component->prev->next = component->next; }

	component->go = 0;
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
