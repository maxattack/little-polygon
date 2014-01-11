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

// Collections are maintained using an intrusive doubly-linked list between
// pre-allocated slots - no dynamic memory after init.

struct GoComponentSlot : GoComponent {
	GoComponentSlot *prev;
	GoComponentSlot *next;
};

struct GoSlot {
	GO go; // GO is a mangled-index to reduce ID dupping
	union {
		uint32_t flags;
		struct {
			uint32_t allocated : 1;
			uint32_t enabled : 1;
		};
	};
	uint32_t nameHash;
	const char *name;
	vec2 position;
	GoComponentSlot *coFirst;
	GoSlot *prev;
	GoSlot *next;
};

struct GoComponenTypeSlot {
	GoMessageHandler handler;
};

struct GoContext {
	size_t goCapacity;
	size_t goCount;
	size_t coCapacity;
	size_t coCount;
	size_t typeCapacity;

	GoSlot *goFirst;
	GoSlot *goFirstFree;
	GoComponentSlot *coFirstFree;
};

#define GO_INDEX(go) (0xffff & go)
#define GO_FINGERPRINT(go) ((0xffff0000 & go) >> 16)

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

GoContext *createGoContext(size_t goCapacity, size_t coCapacity, size_t coTypeCapacity) {
	ASSERT(goCapacity < 0xffff);

	// block allocate the memory
	GoContext *context = (GoContext*) LITTLE_POLYGON_MALLOC(
		sizeof(GoContext) + 
		goCapacity * sizeof(GoSlot) + 
		coTypeCapacity * sizeof(GoComponenTypeSlot) +
		coCapacity * sizeof(GoComponentSlot)
	);
	auto goBuf = (GoSlot*) (context+1);
	auto coTypeBuf = (GoComponenTypeSlot*) (goBuf + goCapacity);
	auto coBuf = (GoComponentSlot*) (coTypeBuf + coTypeCapacity);

	// initialize fields
	context->goCapacity = goCapacity;
	context->goCount = 0;
	context->coCapacity = coCapacity;
	context->coCount = 0;
	context->typeCapacity = coTypeCapacity;
	context->goFirst = 0;
	context->goFirstFree = goBuf;
	context->coFirstFree = coBuf;
	
	// initialize free-slot linked lists
	for(size_t i=0; i<goCapacity; ++i) {
		goBuf[i].go = 0x10000;
		goBuf[i].flags = 0;
		goBuf[i].prev = i == 0 ? 0 : &goBuf[i-1];
		goBuf[i].next = i == goCapacity-1 ? 0 : &goBuf[i+1];
	}
	for(size_t i=0; i<coCapacity; ++i) {
		coBuf[i].prev = i == 0 ? 0 : &coBuf[i-1];
		coBuf[i].next = i == coCapacity-1 ? 0 : &coBuf[i+1];
	}
	for(size_t i=0; i<coTypeCapacity; ++i) {
		coTypeBuf[i].handler = 0;
	}
	
	return context;
}

void destroy(GoContext *context) {
	
	// tear-down all the game objects
	while(context->goFirst) {
		destroy(context, context->goFirst->go);
	}
	
	// de-alloc the memory
	LITTLE_POLYGON_FREE(context);
}

GO createGameObject(GoContext *context, const char* name, float x, float y, GO result) {
	
	// check capacity
	if (context->goCount == context->goCapacity) {
		return 0;
	}

	// determine slot
	GoSlot *slot = 0;
	auto goBuf = (GoSlot*)(context+1);
	if (result) {
		// verify that the slot is free
		auto index = GO_INDEX(result);
		ASSERT(index < context->goCapacity);
		slot = goBuf + index;
		if (slot->allocated) {
			return 0;
		} else {
			slot->go = result;
		}
	} else {
		// generate a new ID
		slot = context->goFirstFree;
		result = slot->go;
	}

	// remove from free list 
	if (slot == context->goFirstFree) { context->goFirstFree = context->goFirstFree->next; }
	if (slot->prev) { slot->prev->next = slot->next; }
	if (slot->next) { slot->next->prev = slot->prev; }

	// prepend to active list
	slot->prev = 0;
	slot->next = context->goFirst;
	if (context->goFirst) { context->goFirst->prev = slot; }
	context->goFirst = slot;

	// initialize fields
	slot->allocated = 1;
	slot->nameHash = hash(name);
	slot->name = name;
	slot->position = vec(x,y);
	slot->coFirst = 0;

	// finalize bookkeeping and return
	++context->goCount;
	return result;
}

void destroy(GoContext *context, GO go) {
	
	// lookup slot
	auto goBuf = (GoSlot*)(context+1);
	auto index = GO_INDEX(go);
	ASSERT(index < context->goCapacity);
	auto slot = goBuf  + index;
	ASSERT(slot->allocated);

	// destroy components
	while(slot->coFirst) {
		removeComponent(context, slot->coFirst);
	}

	// remove from active list
	if (slot == context->goFirst) { context->goFirst = context->goFirst->next; }
	if (slot->prev) { slot->prev->next = slot->next; }
	if (slot->next) { slot->next->prev = slot->prev; }

	// prepend to free list
	slot->prev = 0;
	slot->next = context->goFirstFree;
	if (context->goFirstFree) { context->goFirstFree->prev = slot; }
	context->goFirstFree = slot;

	// udpate fields
	slot->flags = 0;
	slot->go += 0x10000; // fingerprint the ID to reduce duping
}


GO find(GoContext *context, const char *name) {
	// pretty dumb O(N) lookup - not meant for mission-critical code
	auto fnv = hash(name);
	for(auto p=context->goFirst; p; p=p->next) {
		if (p->nameHash == fnv && strcmp(name, p->name) == 0) {
			return p->go;
		}
	}
	return 0;
}

static GoSlot* slotFor(GoContext *context, GO go) {
	auto goBuf = (GoSlot*)(context+1);
	auto index = GO_INDEX(go);
	ASSERT(index < context->goCapacity);
	auto slot = goBuf  + index;
	ASSERT(go == slot->go && slot->allocated);
	return slot;	
}

bool goEnabled(GoContext *context, GO go) {
	return slotFor(context, go)->enabled;	
}

const char* goName(GoContext *context, GO go) {
	return slotFor(context, go)->name;
}

vec2 goPosition(GoContext *context, GO go) {
	return slotFor(context, go)->position;	
}

void setPosition(GoContext *context, GO go, vec2 p) {
	slotFor(context, go)->position = p;	
}

static GoComponenTypeSlot* coTypeSlotFor(GoContext *context, CID cid) {
	ASSERT(cid < context->typeCapacity);
	auto goBuf = (GoSlot*)(context+1);
	auto tpBuf = (GoComponenTypeSlot*)(goBuf + context->goCapacity);
	return tpBuf + cid;
}

void registerComponent(GoContext *context, CID cid, GoMessageHandler handler) {
	auto coSlot  = coTypeSlotFor(context, cid);
	ASSERT(coSlot->handler == 0);
	coSlot->handler = handler;
}

GoComponent *addComponent(GoContext *context, GO go, CID cid, const void *data) {
	ASSERT(context->coCount < context->coCapacity);
	auto slot = slotFor(context, go);
	auto coSlot  = coTypeSlotFor(context, cid);
	ASSERT(coSlot->handler != 0);

	// pop a slot from the component free list
	auto result = context->coFirstFree;
	context->coFirstFree = context->coFirstFree->next;
	if (result->next) { result->next->prev = result->prev; }
	if (result->prev) { result->prev->next = result->next; }

	// prepend to go's component list
	result->prev = 0;
	result->next = slot->coFirst;
	if (slot->coFirst) { slot->coFirst->prev = result; }
	slot->coFirst = result;

	// init fields
	result->cid = cid;
	result->go = slot->go;
	result->data = data;
	result->userData = 0;
	context->coCount++;

	// logically initialize
	coSlot->handler(result, GO_MESSAGE_INIT, 0);
	if (slot->enabled) {
		coSlot->handler(result, GO_MESSAGE_ENABLE, 0);
	}

	// update bookkeeping and return
	return result;
}

GoComponent *getComponent(GoContext *context, GO go, CID cid) {
	for(auto p=slotFor(context, go)->coFirst; p; p=p->next) {
		if (p->cid == cid) {
			return p;
		}
	}	

	return 0;
}

void removeComponent(GoContext *context, GoComponent *component) {
	auto slot = slotFor(context, component->go);
	auto coTypeSlot  = coTypeSlotFor(context, component->cid);
	ASSERT(coTypeSlot->handler != 0);

	// logically destroy
	coTypeSlot->handler(component, GO_MESSAGE_DESTROY, 0);

	// remove from GO list
	auto coSlot = (GoComponentSlot*) component;
	if (coSlot == slot->coFirst) { slot->coFirst = slot->coFirst->next; }
	if (coSlot->next) { coSlot->next->prev = coSlot->prev; }
	if (coSlot->prev) { coSlot->prev->next = coSlot->next; }

	// add to free list
	coSlot->prev = 0;
	coSlot->next = context->coFirstFree;
	if (context->coFirstFree) { context->coFirstFree->prev = coSlot; }
	context->coFirstFree = coSlot;
}

static void sendMessage(GoContext *context, GoComponent *component, uint32_t messageId, void *params) {
	auto coTypeSlot  = coTypeSlotFor(context, component->cid);
	ASSERT(coTypeSlot->handler != 0);
	coTypeSlot->handler(component, messageId, params);
}

void enable(GoContext *context, GO go) {
	auto goBuf = (GoSlot*)(context+1);
	auto index = GO_INDEX(go);
	ASSERT(index < context->goCapacity);
	auto slot = goBuf  + index;
	ASSERT(slot->allocated);

	if (!slot->enabled) {
		for (auto p=slot->coFirst; p; p=p->next) {
			sendMessage(context, p, GO_MESSAGE_ENABLE, 0);
		}
		slot->enabled = 1;
	}


}

void disable(GoContext *context, GO go) {
	auto slot = slotFor(context, go);
	if (slot->enabled) {
		for (auto p=slot->coFirst; p; p=p->next) {
			sendMessage(context, p, GO_MESSAGE_DISABLE, 0);
		}
		slot->enabled = 0;
	}
}

void sendMessage(GoContext *context, GO go, uint32_t messageId, void *params) {
	auto slot = slotFor(context, go);	
	for(auto p=slot->coFirst; p; p=p->next) {
		sendMessage(context, p, messageId, params);
	}
}

GoIterator::GoIterator(GoContext *context) : internal(context->goFirst), current(context->goFirst ? context->goFirst->go : 0) {
}

void GoIterator::next() { 
	auto slot = (GoSlot*) internal;
	slot = slot->next;
	current = slot ? slot->go : 0;
}

GoComponentIterator::GoComponentIterator(GoContext *context, GO go) {
	auto goBuf = (GoSlot*)(context+1);
	auto index = GO_INDEX(go);
	ASSERT(index < context->goCapacity);
	auto slot = goBuf  + index;
	ASSERT(slot->allocated);
	current = slot->coFirst;	
}

void GoComponentIterator::next() { 
	current = ((GoComponentSlot*)current)->next; 
}




