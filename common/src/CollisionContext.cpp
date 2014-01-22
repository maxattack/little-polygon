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

#include "littlepolygon_collisions.h"
#include "littlepolygon_bitset.h"

typedef Bitset<1024> ColliderSet;

#define DELEGATE_NONE    0
#define DELEGATE_NODE    1
#define DELEGATE_XFORM   2
#define DELEGATE_GENERIC 3

//------------------------------------------------------------------------------
// INNER TYPES
//------------------------------------------------------------------------------

struct Collider {

	// would it improve performance to refactor this into a
	// "structure-of-arrays" so that geometric fields are
	// separated from logical fields?

	CollisionContext *context;
	AABB box;
	vec2 pivot;
	uint32_t categoryMask;
	uint32_t collisionMask;
	uint32_t triggerMask;
	union {
		uint32_t flags;
		struct {
			uint32_t enabled : 1;
			uint32_t delegateType : 31;
		};
	};
	union {
		void *delegate;
		FkNodeRef nodeDelegate;
		AffineMatrix *matrixDelegate;
		ColliderDelegate *genericDelegate;
	};
	void *userData;
};

struct Contact {
	Collider *collider;
	Collider *trigger;
};

struct Bucket {
	ColliderSet mask;
};

struct CollisionContext {
	int colliderCapacity;
	int contactCapacity;
	int bucketCount;
	int colliderCount;
	int contactCount;

	AffineMatrix metersToDisplay;

	ColliderSet alloc;

	Collider slots[1];
	Bucket *buckets() { return (Bucket*)(slots + colliderCapacity); }
	Contact *contacts() { return (Contact*)(buckets() + bucketCount); }

	inline Collider *slot(int i) {
		return slots + i;
	}

	inline Contact *contact(int i) {
		return contacts() + i;
	}

};

//------------------------------------------------------------------------------
// MAFFEMATICS
//------------------------------------------------------------------------------

float Ray::intersect(const AABB& box) const {
	float result = -1;
	
	if (p0.x < box.p0.x && p1.x > box.p0.x) {
		// check left
		float u = (box.p0.x - p0.x) / (p1.x - p0.x);
		float y = p0.y + u * (p1.y - p0.y);
		if (y > box.p0.y && y < box.p1.y) { result = u; }
	} else if (p0.x > box.p1.x && p1.x < box.p1.x) {
		// check right
		float u = (box.p1.x - p0.x) / (p1.x - p0.x);
		float y = p0.y + u * (p1.y - p0.y);
		if (y > box.p0.y && y < box.p1.y) { result = u; }
	}

	if (p0.y < box.p0.y && p1.y > box.p0.y) {
		// check top
		float u = (box.p0.y - p0.y) / (p1.y - p0.y);
		float x = p0.x + u * (p1.x - p0.x);
		if (x > box.p0.x && x < box.p1.x) { result = result > 0 ? MIN(result,u) : u; }
	} else if (p0.y > box.p1.y && p1.y < box.p1.y) {
		// check bottom
		float u = (box.p1.y - p0.y) / (p1.y - p0.y);
		float x = p0.x + u * (p1.x - p0.x);
		if (x > box.p0.x && x < box.p1.x) { result = result > 0 ? MIN(result,u) : u; }
	}

	return result;
}

//------------------------------------------------------------------------------
// COLLISION CONTEXT METHODS
//------------------------------------------------------------------------------

CollisionContext *createCollisionSystem(size_t colliderCapacity, size_t numBuckets, size_t maxContacts) {
	ASSERT(colliderCapacity <= 1024);
	ASSERT(numBuckets <= 1024);
	ASSERT(maxContacts <= 1024);

	// block-alloc memory
	auto context = (CollisionContext*) LITTLE_POLYGON_MALLOC(
		sizeof(CollisionContext) + 
		sizeof(Collider) * (colliderCapacity-1) + 
		sizeof(Bucket) * (numBuckets) +
		sizeof(Contact) * (maxContacts)
	);

	// init fields
	context->colliderCapacity = colliderCapacity;
	context->contactCapacity = maxContacts;
	context->bucketCount = numBuckets;
	context->colliderCount = 0;
	context->contactCount = 0;
	context->metersToDisplay = affineIdentity();
	context->alloc.reset();

	// clear buckets
	auto bucketBuf = context->buckets();
	for(int i=0; i<numBuckets; ++i) {
		bucketBuf[i].mask.reset();
	}

	return context;
}

void CollisionSystemRef::destroy() {
	LITTLE_POLYGON_FREE(context);
}

const AffineMatrix& CollisionSystemRef::metersToDisplay() const {
	return context->metersToDisplay;
}

void CollisionSystemRef::setMetersToDisplay(const AffineMatrix& matrix) {
	context->metersToDisplay = matrix;
}

static ColliderSet& bucketFor(CollisionContext *context, int x, int y) {
	// inlined fnv-1a
	return (context->buckets() + (
		((((0x811c9dc5 ^ x) * 0x01000193) ^ y) * 0x01000193) % context->bucketCount
	))->mask;

}

static void doHash(Collider *c) {
	int minX = int(c->box.p0.x+0.5f);
	int minY = int(c->box.p0.y+0.5f);
	int maxX = int(c->box.p1.x+0.5f);
	int maxY = int(c->box.p1.y+0.5f);
	unsigned slot = (c - c->context->slots);
	for(int x=minX; x<=maxX; ++x)
	for(int y=minY; y<=maxY; ++y) {
		bucketFor(c->context, x, y).mark(slot);
	}	
}

static void doUnhash(Collider *c) {
	int minX = int(c->box.p0.x+0.5f);
	int minY = int(c->box.p0.y+0.5f);
	int maxX = int(c->box.p1.x+0.5f);
	int maxY = int(c->box.p1.y+0.5f);
	unsigned slot = (c - c->context->slots);
	for(int x=minX; x<=maxX; ++x)
	for(int y=minY; y<=maxY; ++y) {
		bucketFor(c->context, x, y).clear(slot);
	}
}

Collider *CollisionSystemRef::addCollider(
	const AABB& box, 
	uint32_t categoryMask, 
	uint32_t collisionMask,
	uint32_t triggerMask,
	bool enabled,
	void *userData
) {
	// allocate slot
	ASSERT(context->colliderCount < context->colliderCapacity);
	unsigned idx;
	(~context->alloc).findFirst(idx);
	context->alloc.mark(idx);

	// init fields
	auto result = context->slots + idx;
	result->context = context;
	result->box = box;
	result->pivot = vec(0,0);
	result->categoryMask = categoryMask;
	result->collisionMask = collisionMask;
	result->triggerMask = triggerMask;
	result->flags = 0;
	result->delegate = 0;
	result->userData = userData;

	// hash
	if (enabled) {
		doHash(result);
		result->enabled = 1;
	}

	return result;
}

void doBroadPhase(CollisionContext *context, const AABB& sweep, ColliderSet& outResult) {
	// identify sectors
	int minX = int(sweep.p0.x+0.5f);
	int minY = int(sweep.p0.y+0.5f);
	int maxX = int(sweep.p1.x+0.5f);
	int maxY = int(sweep.p1.y+0.5f);

	// union buckets for those sectors
	for(int x=minX; x<=maxX; ++x)
	for(int y=minY; y<=maxY; ++y) {
		outResult |= bucketFor(context, x, y);
	}

}

int CollisionSystemRef::query(const AABB& box, uint32_t mask, int outCapacity, ColliderRef *resultBuf) {
	ASSERT(outCapacity > 0);
	ColliderSet candidates;
	doBroadPhase(context, box, candidates);
	int nResults = 0;
	unsigned slot;
	for(auto i=candidates.listBits(); i.next(slot);) {
		auto collider = context->slot(slot);
		if ((collider->categoryMask & mask) != 0 && collider->box.overlaps(box)) {
			resultBuf[nResults] = collider;
			nResults++;
			if (nResults == outCapacity) {
				return nResults;
			}
		}
	}
	return nResults;	
}

float CollisionSystemRef::raycast(const Ray& ray, uint32_t mask, ColliderRef *result) {
	// this impl is pretty naive - we just test the ray against all the boxes in it's
	// bounding box.  Possible improvements:
	//   - use smaller bounding boxes / sub-stepping?
	//   - iterate through sectors bresenham-style?
	ColliderSet candidates;
	AABB box(
		MIN(ray.p0.x, ray.p1.x),
		MIN(ray.p0.y, ray.p1.y),
		MAX(ray.p0.x, ray.p1.x),
		MAX(ray.p0.y, ray.p1.y)
	);
	doBroadPhase(context, box, candidates);
	unsigned slot;
	float u = MAXFLOAT;
	*result = 0;
	for(auto i=candidates.listBits(); i.next(slot);) {
		auto collider = context->slot(slot);
		if (collider->categoryMask & mask) {
			float v = ray.intersect(collider->box);
			if (v > 0 && v < u) {
				u = v;
				*result = collider;
			}
		}
	}
	return (*result) ? u : -1;
}

void CollisionSystemRef::debugDraw(LinePlotterRef plotter, Color color) {
	unsigned slot;
	for(auto i=context->alloc.listBits(); i.next(slot);) {
		auto c = context->slot(slot);
		plotter.plot(
			context->metersToDisplay.transformPoint(c->box.topLeft()), 
			context->metersToDisplay.transformPoint(c->box.topRight()), 
			color
		);
		plotter.plot(
			context->metersToDisplay.transformPoint(c->box.topRight()), 
			context->metersToDisplay.transformPoint(c->box.bottomRight()), 
			color
		);
		plotter.plot(
			context->metersToDisplay.transformPoint(c->box.bottomRight()), 
			context->metersToDisplay.transformPoint(c->box.bottomLeft()), 
			color
		);
		plotter.plot(
			context->metersToDisplay.transformPoint(c->box.bottomLeft()), 
			context->metersToDisplay.transformPoint(c->box.topLeft()), 
			color
		);
	}	
}

//------------------------------------------------------------------------------
// COLLIDER METHODS
//------------------------------------------------------------------------------

CollisionSystemRef ColliderRef::context() {
	return collider->context;
}

void ColliderRef::destroy() {
	auto context = collider->context;
	unsigned slot = collider - context->slots;
	ASSERT(context->alloc[slot]);

	// // remove relevant contacts
	int i = context->contactCount;
	while(i>0) {
		--i;
		if (context->contact(i)->collider == collider || context->contact(i)->trigger == collider) {
			if (i != context->contactCount-1) {
				*context->contact(i) = *context->contact(context->contactCount-1);
			}
			--context->contactCount;
		}
	}

	if (collider->enabled) {
		doUnhash(collider);
	}
	context->alloc.clear(slot);

}

Collision ColliderRef::move(vec2 offset) {
	auto context = collider->context;

	// remove this from the hash because (i) it might change and (ii) we don't want
	// to collider with ourselves, anyway
	if (collider->enabled) {
		doUnhash(collider);
	}

	// Udpate along axis separately, so that we "slide" along
	// parallel surfaces and fix snugly into corners.  The result
	// is the amount that we actually moved, after clipping.
	Collision result;
	result.hit = 0;
	auto size = collider->box.size();

	AABB box = collider->box;
	if (offset.x > 0) { box.p1.x += offset.x; }
	else { box.p0.x += offset.x; }
	if (offset.y > 0) { box.p1.y += offset.y; }
	else { box.p0.y += offset.y; }
	ColliderSet candidates;
	doBroadPhase(context, box, candidates);
	unsigned slot;

	if (offset.y > 0) {
		
		// moving down
		collider->box.p1.y += offset.y;
		for(auto i=candidates.listBits(); i.next(slot);) {
			auto c = context->slots + slot;
			if (collides(c)) {
				collider->box.p1.y = c->box.top();
				result.hitBottom = true;
			}
		}
		collider->box.p0.y = collider->box.p1.y - size.y;

	} else if (offset.y < 0) {

		// moving up
		collider->box.p0.y += offset.y;
		for(auto i=candidates.listBits(); i.next(slot);) {
			auto c = context->slots + slot;
			if (collides(c)) {
				collider->box.p0.y = c->box.bottom();
				result.hitTop = true;
			}
		}
		collider->box.p1.y = collider->box.p0.y + size.y;

	}

	if (offset.x > 0) {
		
		// moving right
		collider->box.p1.x += offset.x;
		for(auto i=candidates.listBits(); i.next(slot);) {
			auto c = context->slots + slot;
			if (collides(c)) {
				collider->box.p1.x = c->box.left();
				result.hitRight = true;
			}
		}
		collider->box.p0.x = collider->box.p1.x - size.x;

	} else if (offset.x < 0) {

		// moving left
		collider->box.p0.x += offset.x;
		for(auto i=candidates.listBits(); i.next(slot);) {
			auto c = context->slots + slot;
			if (collides(c)) {
				collider->box.p0.x = c->box.right();
				result.hitLeft = true;
			}
		}
		collider->box.p1.x = collider->box.p0.x + size.x;

	}

	if (collider->enabled) {
		doHash(collider);
	}
	
	if (collider->delegateType) {
		auto p = context->metersToDisplay
			.transformPoint(collider->box.p0 + collider->pivot);
		switch(collider->delegateType) {
			case DELEGATE_NODE: {
				auto world = collider->nodeDelegate.world();
				world.t = p;
				collider->nodeDelegate.setWorld(world);
				break;
			}
			case DELEGATE_XFORM:
				collider->matrixDelegate->t = p;
				break;
			case DELEGATE_GENERIC:
				collider->genericDelegate->setPosition(p);
				break;
			default:
				break;
		}
	}

	return result;	
}

void ColliderRef::setPosition(vec2 topLeft) {
	if (collider->enabled) {
		doUnhash(collider);
	}
	auto sz = collider->box.size();
	collider->box.p0 = topLeft;
	collider->box.p1 = topLeft + sz;
	if (collider->enabled) {
		doHash(collider);
	}
}


int ColliderRef::queryTriggers(int outCapacity, TriggerEvent *resultBuf) {
	ASSERT(outCapacity > 0);
	auto context = collider->context;
	Collider *c = collider;
	int nResults = 0;

	// identify the contacts that this collider participates in
	ColliderSet contactSet;
	for(int i=0; i<context->contactCount; ++i) {
		if (context->contact(i)->collider == c) {
			contactSet.mark(i);
		}
	}

	// inner helper methods
	auto findContact = [&](Collider *trig) {
		unsigned contactIndex;
		for(auto i=contactSet.listBits(); i.next(contactIndex);) {
			if (context->contact(contactIndex)->trigger == trig) {
				return int(contactIndex);
			}
		}
		return context->contactCount;
	};

	auto pushResult = [&nResults, resultBuf](TriggerEvent::TriggerType typ, Collider *trig) {
		resultBuf[nResults].type = typ;
		resultBuf[nResults].trigger = trig;
		++nResults;
	};

	// identify overlapping triggers (ENTER and STAY)
	ColliderSet candidates;
	doBroadPhase(context, c->box, candidates);
	unsigned slot;
	for(auto i=candidates.listBits(); i.next(slot);) {
		auto trigger = context->slots + slot;
		if (triggers(trigger)) {
			int i = findContact(trigger);
			contactSet.clear(i);
			if (i < context->contactCount) {
				// leave this out unless there's a compelling reason
				// to include it - feels like unnecessary noise/copying
				// pushResult(Trigger::STAY, trigger);
				// if (nResults == outCapacity) { return nResults; }
			} else {
				ASSERT(context->contactCount < context->contactCapacity);
				context->contactCount++;
				context->contact(i)->collider = c;
				context->contact(i)->trigger = trigger;
				pushResult(TriggerEvent::ENTER, trigger);
				if (nResults == outCapacity) { return nResults; }
			}
		}
	}

	// contacts may be re-ordered in processing
	uint32_t oldToNewIndex[context->contactCount]; 
	for(int i=0; i<context->contactCount; ++i) {
		oldToNewIndex[i] = i;
	}

	// identify non-ovelapping triggers (EXIT)
	uint32_t contactIndex;
	while(nResults < outCapacity && contactSet.clearFirst(contactIndex)) {
		uint32_t actualIndex = oldToNewIndex[contactIndex];
		pushResult(TriggerEvent::EXIT, context->contact(actualIndex)->trigger);
		--context->contactCount;
		if (actualIndex < context->contactCount) {
			// swap last contact into empty slot
			*context->contact(actualIndex) = *context->contact(context->contactCount);
			oldToNewIndex[context->contactCount] = actualIndex;
		}
	}

	return nResults;	
}


AABB ColliderRef::box() const {
	return collider->box;
}

uint32_t ColliderRef::categoryMask() const {
	return collider->categoryMask;
}

uint32_t ColliderRef::collisionMask() const {
	return collider->collisionMask;
}

uint32_t ColliderRef::triggerMask() const {
	return collider->triggerMask;
}

vec2 ColliderRef::pivot() const {
	return collider->pivot;
}

void *ColliderRef::userData() const {
	return collider->userData;
}

void ColliderRef::enable() {
	if (!collider->enabled) {
		collider->enabled = 1;
		doHash(collider);
	}
}

void ColliderRef::disable() {
	if (collider->enabled) {
		collider->enabled = 0;
		doUnhash(collider);
	}
}

void ColliderRef::setDelegate(AffineMatrix *xform, vec2 pivot) {
	collider->delegateType = DELEGATE_XFORM;
	collider->pivot = pivot;
	collider->matrixDelegate = xform;
}

void ColliderRef::setDelegate(FkNodeRef node, vec2 pivot) {
	collider->delegateType = DELEGATE_NODE;
	collider->pivot = pivot;
	collider->nodeDelegate = node;
}

void ColliderRef::setDelegate(ColliderDelegate *delegate, vec2 pivot) {
	collider->delegateType = DELEGATE_GENERIC;
	collider->pivot = pivot;
	collider->genericDelegate = delegate;
}

void ColliderRef::clearDelegate() {
	collider->delegateType = DELEGATE_NONE;
}

void ColliderRef::setCategoryMask(uint32_t mask) {
	collider->categoryMask = mask;
}

void ColliderRef::setCollisionMask(uint32_t mask) {
	collider->collisionMask = mask;
}

void ColliderRef::setTriggerMask(uint32_t mask) {
	collider->triggerMask = mask;
}

void ColliderRef::setPivot(vec2 u) {
	collider->pivot = u;
}

void ColliderRef::setUserData(void *data) {
	collider->userData = data;
}

bool ColliderRef::collides(const ColliderRef c) const {
	return (collider->collisionMask & c.collider->categoryMask) &&
		collider->box.overlaps(c.collider->box);

}

bool ColliderRef::triggers(const ColliderRef c) const {
	return (collider->triggerMask & c.collider->categoryMask) &&
		collider->box.overlaps(c.collider->box);
}
