#include "CollisionSystem.h"
#include <bitset>

// All of these methods are O(n) right now as a first-pass implementation.
// Note that because of their cache-friendliness this may be perfectly
// acceptable.

CollisionSystem::CollisionSystem() {
}

ID CollisionSystem::addCollider(const AABB& box, 
                                uint32_t categoryMask, 
                                uint32_t collisionMask,
                                uint32_t triggerMask,
                                void *aUserData) {
	// create the box
	ASSERT((collisionMask & triggerMask) == 0);
	auto result = colliders.takeOut();
	auto& collider = colliders[result];
	collider.box = box;
	collider.categoryMask = categoryMask;
	collider.collisionMask = collisionMask;
	collider.triggerMask = triggerMask;
	userDataBuf[POOL_SLOT_INDEX(result)] = aUserData;

	return result;
}

void CollisionSystem::removeCollider(ID id) {
	colliders.putBack(id);
}

void CollisionSystem::debugDraw(LinePlotter& plotter) {
	for(auto& c : colliders) {
		plotter.plot(PIXELS_PER_METER * c.box.topLeft(), PIXELS_PER_METER * c.box.topRight(), rgb(0xffff00));
		plotter.plot(PIXELS_PER_METER * c.box.topRight(), PIXELS_PER_METER * c.box.bottomRight(), rgb(0xffff00));
		plotter.plot(PIXELS_PER_METER * c.box.bottomRight(), PIXELS_PER_METER * c.box.bottomLeft(), rgb(0xffff00));
		plotter.plot(PIXELS_PER_METER * c.box.bottomLeft(), PIXELS_PER_METER * c.box.topLeft(), rgb(0xffff00));
	}
}

bool CollisionSystem::move(ID id, vec2 offset, Collision *outResult) {
	
	// Udpate along axis separately, so that we "slide" along
	// parallel surfaces and fix snugly into corners.  The result
	// is the amount that we actually moved, after clipping.

	outResult->offset = offset;
	outResult->hit = 0;
	auto& collider = colliders[id];
	auto size = collider.box.size();

	if (offset.y > 0) {
		
		// moving down
		auto sweep = collider;
		sweep.box.p1.y += offset.y;
		// check against other colliders
		for(auto& c : colliders) {
			if (sweep.collides(c)) {
				sweep.box.p1.y = c.box.top();
				outResult->hitBottom = true;
			}
		}
		// if we hit something, update the bbox
		collider.box.p1.y = sweep.box.p1.y;
		collider.box.p0.y = sweep.box.p1.y - size.y;

	} else if (offset.y < 0) {

		// moving up
		auto sweep = collider;
		sweep.box.p0.y += offset.y;
		// check against other colliders
		for(auto& c : colliders) {
			if (sweep.collides(c)) {
				sweep.box.p0.y = c.box.bottom();
				outResult->hitTop = true;
			}
		}
		// if we hit something update the bbox
		collider.box.p0.y = sweep.box.p0.y;
		collider.box.p1.y = sweep.box.p0.y + size.y;

	}

	if (offset.x > 0) {
		
		// moving right
		auto sweep = collider;
		sweep.box.p1.x += offset.x;
		// check against other colliders
		for(auto& c : colliders) {
			if (sweep.collides(c)) {
				sweep.box.p1.x = c.box.left();
				outResult->hitRight = true;
			}
		}
		// if we hit something, update the bbox
		collider.box.p1.x = sweep.box.p1.x;
		collider.box.p0.x = sweep.box.p1.x - size.x;

	} else if (offset.x < 0) {

		// moving left
		auto sweep = collider;
		sweep.box.p0.x += offset.x;
		// check against other colliders
		for(auto& c : colliders) {
			if (sweep.collides(c)) {
				sweep.box.p0.x = c.box.right();
				outResult->hitLeft = true;
			}
		}
		// if we hit something update the bbox
		collider.box.p0.x = sweep.box.p0.x;
		collider.box.p1.x = sweep.box.p0.x + size.x;

	}

	// do we need to check for triggers?
	outResult->triggerEventCount = 0;
	outResult->triggerEvents = triggerEventBuf;
	if (collider.triggerMask) {

		std::bitset<CONTACT_CAPACITY> activeSet;

		for(auto& c : colliders) {
			if (collider.triggers(c)) {
				int i = findContact(collider.id, c.id);
				activeSet.set(i);
				if (i < ncontacts) {
						
					ASSERT(outResult->triggerEventCount < CONTACT_CAPACITY);
					auto& event = triggerEventBuf[outResult->triggerEventCount];
					event.type = TRIGGER_EVENT_STAY;
					event.trigger = c.id;
					outResult->triggerEventCount++;

				} else {
					ASSERT(ncontacts < CONTACT_CAPACITY);
					ncontacts++;
					contacts[i].collider = collider.id;
					contacts[i].trigger = c.id;

					ASSERT(outResult->triggerEventCount < CONTACT_CAPACITY);
					auto& event = triggerEventBuf[outResult->triggerEventCount];
					event.type = TRIGGER_EVENT_ENTER;
					event.trigger = c.id;
					outResult->triggerEventCount++;
				}
			}
		}

		// traverse in reverse-order so that removing
		// elements does not mess up the index order in activeSet
		int i=ncontacts;
		while(i > 0) {
			--i;
			if (contacts[i].collider == collider.id && !activeSet[i]) {

				ASSERT(outResult->triggerEventCount < CONTACT_CAPACITY);
				auto& event = triggerEventBuf[outResult->triggerEventCount];
				event.type = TRIGGER_EVENT_EXIT;
				event.trigger = contacts[i].trigger;
				outResult->triggerEventCount++;
				
				// swap this with the last element
				ncontacts--;
				if (i < ncontacts) {
					contacts[i] = contacts[ncontacts];
				}
			}
		}

	}

	return outResult->hit;
}

int CollisionSystem::findContact(ID collider, ID trigger) {
	for(int i=0; i<ncontacts; ++i) {
		if (contacts[i].collider == collider && contacts[i].trigger == trigger) {
			return i;
		}
	}
	return ncontacts;
}




