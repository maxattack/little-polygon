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

struct GoComponent {
	GoComponentType *type;
	GameObject *go;
	GoComponent *next;
	void *data;
};

struct GoContext {
	size_t coCapacity;
	size_t goCount;
	size_t coCount;

	FkContext *fkContext;

	GoSet goAllocationMask;
	GoSet goEnabledMask;

	GoComponent *firstFree;

	GameObject gobuf[1024];

	GoComponent *coBuf() { return (GoComponent*)(gobuf + GO_CAPACITY); }

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

GoContext *createGoContext(FkContext *fkContext, size_t coCapacity) {
	// block allocate the memory
	GoContext *context = (GoContext*) LITTLE_POLYGON_MALLOC(
		sizeof(GoContext) + 
		sizeof(GoComponent) * coCapacity
	);

	// initialize fields
	context->coCapacity = coCapacity;
	context->goCount = 0;
	context->coCount = 0;
	context->fkContext = fkContext;	
	context->goAllocationMask =GoSet();
	context->goEnabledMask = GoSet();
	context->firstFree = context->coBuf();

	// initialize component freelist
	auto cbuf = context->coBuf();
	for(int i=0; i<coCapacity-1; ++i) {
		cbuf[i].next = cbuf + (i+1);
	}	
	cbuf[coCapacity-1].next = 0;

	return context;
}

void destroy(GoContext *context) {
	
	// tear-down all the game objects
	unsigned index;
	while(context->goAllocationMask.findFirst(index)) {
		destroy(context->gobuf + index);
	}
	
	// de-alloc the memory
	LITTLE_POLYGON_FREE(context);
}

FkContext *fkContext(GoContext *context) {
	return context->fkContext;
}

GameObject *find(GoContext *context, GameObjectID gid) {
	if (gid == 0 || gid >= GO_CAPACITY) {
		return 0;
	} else {
		return context->gobuf + (gid-1);
	}
}

GameObject *find(GoContext *context, const char *name) {
	// pretty dumb O(N) lookup - not meant for mission-critical code
	auto fnv = hash(name);
	unsigned idx;
	for(auto i=context->goAllocationMask.listBits(); i.next(idx);) {
		GameObject *go = context->gobuf + idx;
		if (go->nameHash == fnv && strcmp(name, go->name) == 0) {
			return go;
		}
	}
	return 0;
}



//------------------------------------------------------------------------------
// GAME OBJECTS
//------------------------------------------------------------------------------

GameObject* addGameObject(GoContext *context, const char* name, GameObjectID explicitId) {
	
	// check capacity
	if (context->goCount == GO_CAPACITY) {
		return 0;
	}

	// determine slot
	unsigned index;
	if (explicitId) {
		// verify that the slot is free
		index = GO_INDEX(explicitId);
		ASSERT(index < GO_CAPACITY);
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
	GameObject *go = context->gobuf + index;

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

static void doDestroy(GameObject *go) {
	// destroy components
	while(go->coFirst) {
		destroy(go->coFirst);
	}
	// update context
	go->context->goAllocationMask.clear(go - go->context->gobuf);
	go->context->goEnabledMask.clear(go - go->context->gobuf);
	--go->context->goCount;	
}

void destroy(GameObject *go) {
	ASSERT(go->context->goAllocationMask[go - go->context->gobuf]);

	// destroy children in bottom-up order (no recursive funciton calls)
	auto iter = FkInvSubtreeIterator(go->node);
	while(!iter.finished()) {
		auto child = iter.ref().data<GameObject>();
		iter.next();
		if (child) {
			doDestroy(child);
		}
	}

	doDestroy(go);
	destroy( go->node );
}

GoContext *goContext(const GameObject *go) {
	return go->context;
}

bool goEnabled(const GameObject *go) {
	return go->context->goEnabledMask[go - go->context->gobuf];
}

const char* goName(const GameObject *go) {
	return go->name;
}

FkNode* goNode(const GameObject *go) {
	return go->node;
}

GoComponent* goComponent(GameObject *go, GoComponentType* type) {
	for(auto c=go->coFirst; c; c=c->next) {
		if (c->type == type) {
			return c;
		}
	}
	return 0;
}

GameObject *goFromNode(FkNode *node) {
	return (GameObject*) fkUserData(node);
}

static int doEnable(GameObject *go) {
	for(auto c=go->coFirst; c; c=c->next) {
		GOSTATUS_CHECK(c->type->enable(c));
	}	
	go->context->goEnabledMask.mark(go - go->context->gobuf);
	return GOSTATUS_OK;
}

int enable(GameObject *go) {
	if (!goEnabled(go)) {
		// enable children after parents (no recursive functions)
		doEnable(go);
		for(FkSubtreeIterator i(go->node); !i.finished(); i.next()) {
			auto child = goFromNode(i.current);
			if (child && !goEnabled(child)) {
				GOSTATUS_CHECK( doEnable(child) );
			}
		}
	}
	return GOSTATUS_OK;
}

static int doDisable(GameObject *go) {
	for(auto c=go->coFirst; c; c=c->next) {
		GOSTATUS_CHECK(c->type->disable(c));
	}	
	go->context->goEnabledMask.clear(go - go->context->gobuf);
	return GOSTATUS_OK;
}

int disable(GameObject *go) {
	if (goEnabled(go)) {
		// disable parents after children (no recursive functions)
		for(FkInvSubtreeIterator i(go->node); !i.finished(); i.next()) {
			auto child = goFromNode(i.current);
			if (child && goEnabled(child)) {
				GOSTATUS_CHECK( doEnable(child) );
			}
		}
		doDisable(go);
	}
	return GOSTATUS_OK;
}

static int doSendMessage(GameObject *go, int messageId, const void *args) {
	for(auto c=go->coFirst; c; c=c->next) {
		GOSTATUS_CHECK(c->type->message(c, messageId, args));
	}	
	go->context->goEnabledMask.clear(go - go->context->gobuf);
	return GOSTATUS_OK;	
}

int sendMessage(GameObject *go, int messageId, const void *args) {
	if (goEnabled(go)) {
		GOSTATUS_CHECK( doSendMessage(go, messageId, args) );
		for(FkSubtreeIterator i(go->node); !i.finished(); i.next()) {
			auto child = goFromNode(i.current);
			if (child && goEnabled(child)) {
				GOSTATUS_CHECK( doSendMessage(child, messageId, args) );
			}
		}
	}
	return GOSTATUS_OK;
}

//------------------------------------------------------------------------------
// COMPONENTS
//------------------------------------------------------------------------------

GoComponent* addComponent(GameObject *go, GoComponentType* type, const void* args) {
	// alloc record
	ASSERT(go->context->coCount < go->context->coCapacity);
	auto component = go->context->firstFree;
	go->context->firstFree = component->next;
	component->type = type;
	component->go = go;
	component->data = 0;

	// we apply a heuristic here - the first component that's added will always
	// be at the head of the list.  This is because it's typically looked up 
	// frequently - it's most commonly a transform component - so we make sure it
	// doesn't get pushed to the end of the list, causing all lookups on it to be
	// worst-case.
	if (go->coFirst) {
		component->next = go->coFirst->next;
		go->coFirst->next = component;
	} else {
		component->next = 0;
		go->coFirst = component;
	}

	// init fields
	component->type->init(component, args);
	if (goEnabled(go)) {
		component->type->enable(component);
	}

	return component;
}

void destroy(GoComponent *component) {
	// logically destroy
	component->type->release(component);

	// remove from GO list
	if (component == component->go->coFirst) {
		component->go->coFirst = component->next;
	} else {
		auto c = component->go->coFirst;
		while(c->next != component) { c = c->next; }
		c->next = component->next;
	}

	// preprend to freelist
	component->next = component->go->context->firstFree;
	component->go->context->firstFree = component;
}

GoComponentType* coType(GoComponent *component) {
	return component->type;
}

void *coHandle(GoComponent *component) {
	return component->data;
}

GameObject* coObject(GoComponent *component) {
	return component->go;
}

void setHandle(GoComponent *component, void *handle) {
	component->data = handle;
}

//------------------------------------------------------------------------------
// ITERATORS
//------------------------------------------------------------------------------

GoIterator::GoIterator(GoContext *context) : 
current(0),
internal(context->goAllocationMask.listBits()) {
	unsigned index;
	if (internal.next(index)) {
		current = context->gobuf + index;
	}
}

void GoIterator::next() { 
	unsigned index;
	current = internal.next(index) ? current->context->gobuf + index : 0;
}


GoComponentIterator::GoComponentIterator(GameObject *go) : current(go->coFirst) {
}

void GoComponentIterator::next() {
	current = current->next;
}

