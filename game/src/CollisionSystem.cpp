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

#include "CollisionSystem.h"
#include <bitset>

CollisionSystem::CollisionSystem() {
	freeSlots.mark();
}

Collider* CollisionSystem::addCollider(
	const AABB& box, 
	uint32_t categoryMask, 
	uint32_t collisionMask,
	uint32_t triggerMask,
	void *userData
) {
	ASSERT(freeSlots.count() > 0);
	ASSERT((triggerMask & collisionMask) == 0);

	// allocate a box from the lowest-index free slot
	// (we're trying to stay cache-coherent)
	unsigned slot;
	freeSlots.clearFirst(slot);

	// initialize fields
	Collider& result = slots[slot];
	result.box = box;
	result.categoryMask = categoryMask;
	result.collisionMask = collisionMask;
	result.triggerMask = triggerMask;
	result.userData = userData;

	// add to buckets
	hash(&result);

	return &result;
}

Collision CollisionSystem::move(Collider *collider, vec2 offset) {
	// remove this from the hash because (i) it might change and (ii) we don't want
	// to collider with ourselves, anyway
	unhash(collider);

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
	broadPhase(box, candidates);
	unsigned slot;

	if (offset.y > 0) {
		
		// moving down
		collider->box.p1.y += offset.y;
		for(auto i=candidates.listBits(); i.next(slot);) {
			auto c = slots + slot;
			if (collider->collides(c)) {
				collider->box.p1.y = c->box.top();
				result.hitBottom = true;
			}
		}
		collider->box.p0.y = collider->box.p1.y - size.y;

	} else if (offset.y < 0) {

		// moving up
		collider->box.p0.y += offset.y;
		for(auto i=candidates.listBits(); i.next(slot);) {
			auto c = slots + slot;
			if (collider->collides(c)) {
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
			auto c = slots + slot;
			if (collider->collides(c)) {
				collider->box.p1.x = c->box.left();
				result.hitRight = true;
			}
		}
		collider->box.p0.x = collider->box.p1.x - size.x;

	} else if (offset.x < 0) {

		// moving left
		collider->box.p0.x += offset.x;
		for(auto i=candidates.listBits(); i.next(slot);) {
			auto c = slots + slot;
			if (collider->collides(c)) {
				collider->box.p0.x = c->box.right();
				result.hitLeft = true;
			}
		}
		collider->box.p1.x = collider->box.p0.x + size.x;

	}

	hash(collider);
	return result;
}

int CollisionSystem::queryTriggers(Collider *c, int outCapacity, Trigger *resultBuf) {
	ASSERT(outCapacity > 0);
	int nResults = 0;

	// identify the contacts that this collider participates in
	Bitset<CONTACT_CAPACITY> contactSet;
	for(int i=0; i<nContacts; ++i) {
		if (contacts[i].collider == c) {
			contactSet.mark(i);
		}
	}

	// inner helper methods
	auto findContact = [this, &contactSet, &c](Collider *trig) {
		unsigned contactIndex;
		for(auto i=contactSet.listBits(); i.next(contactIndex);) {
			if (this->contacts[contactIndex].trigger == trig) {
				return contactIndex;
			}
		}
		return this->nContacts;
	};

	auto pushResult = [&nResults, resultBuf](Trigger::TriggerType typ, Collider *trig) {
		resultBuf[nResults].type = typ;
		resultBuf[nResults].trigger = trig;
		++nResults;
	};

	// identify overlapping triggers (ENTER and STAY)
	ColliderSet candidates;
	broadPhase(c->box, candidates);
	unsigned slot;
	for(auto i=candidates.listBits(); i.next(slot);) {
		auto trigger = slots + slot;
		if (c->triggers(trigger)) {
			int i = findContact(trigger);
			contactSet.clear(i);
			if (i < nContacts) {
				// leave this out unless there's a compelling reason
				// to include it - feels like unnecessary noise/copying
				// pushResult(Trigger::STAY, trigger);
				// if (nResults == outCapacity) { return nResults; }
			} else {
				ASSERT(nContacts < CONTACT_CAPACITY);
				nContacts++;
				contacts[i].collider = c;
				contacts[i].trigger = trigger;
				pushResult(Trigger::ENTER, trigger);
				if (nResults == outCapacity) { return nResults; }
			}
		}
	}

	// contacts may be re-ordered in processing
	uint32_t oldToNewIndex[nContacts]; 
	for(int i=0; i<nContacts; ++i) {
		oldToNewIndex[i] = i;
	}

	// identify non-ovelapping triggers (EXIT)
	uint32_t contactIndex;
	while(nResults < outCapacity && contactSet.clearFirst(contactIndex)) {
		uint32_t actualIndex = oldToNewIndex[contactIndex];
		pushResult(Trigger::EXIT, contacts[actualIndex].trigger);
		--nContacts;
		if (actualIndex < nContacts) {
			// swap last contact into empty slot
			contacts[actualIndex] = contacts[nContacts];
			oldToNewIndex[nContacts] = actualIndex;
		}
	}

	return nResults;
}

int CollisionSystem::queryColliders(const AABB& box, uint32_t mask, int outCapacity, Collider **resultBuf) {
	ASSERT(outCapacity > 0);
	ColliderSet candidates;
	broadPhase(box, candidates);
	int nResults = 0;
	unsigned slot;
	for(auto i=candidates.listBits(); i.next(slot);) {
		auto& collider = slots[slot];
		if ((collider.categoryMask & mask) != 0 && collider.box.overlaps(box)) {
			resultBuf[nResults] = &collider;
			nResults++;
			if (nResults == outCapacity) {
				return nResults;
			}
		}
	}
	return nResults;
}

void CollisionSystem::removeCollider(Collider *collider) {
	unsigned slot = collider - slots;
	ASSERT(!freeSlots[slot]);

	// // remove relevant contacts
	int i = nContacts;
	while(i>0) {
		--i;
		if (contacts[i].collider == collider || contacts[i].trigger == collider) {
			if (i != nContacts-1) {
				contacts[i] = contacts[nContacts-1];
			}
			--nContacts;
		}
	}

	unhash(collider);
	freeSlots.mark(slot);
}

CollisionSystem::ColliderSet& CollisionSystem::bucketFor(int x, int y) {
	// inlined fnv-1a
	return buckets[
		((((0x811c9dc5 ^ x) * 0x01000193) ^ y) * 0x01000193) % BUCKET_COUNT
	];
}

void CollisionSystem::hash(Collider *c) {
	int minX = int(c->box.p0.x+0.5f);
	int minY = int(c->box.p0.y+0.5f);
	int maxX = int(c->box.p1.x+0.5f);
	int maxY = int(c->box.p1.y+0.5f);
	unsigned slot = (c - slots);
	for(int x=minX; x<=maxX; ++x)
	for(int y=minY; y<=maxY; ++y) {
		bucketFor(x,y).mark(slot);
	}
}

void CollisionSystem::unhash(Collider *c) {
	int minX = int(c->box.p0.x+0.5f);
	int minY = int(c->box.p0.y+0.5f);
	int maxX = int(c->box.p1.x+0.5f);
	int maxY = int(c->box.p1.y+0.5f);
	unsigned slot = (c - slots);
	for(int x=minX; x<=maxX; ++x)
	for(int y=minY; y<=maxY; ++y) {
		bucketFor(x,y).clear(slot);
	}
}

void CollisionSystem::broadPhase(const AABB& sweep, ColliderSet& outResult) {
	// identify sectors
	int minX = int(sweep.p0.x+0.5f);
	int minY = int(sweep.p0.y+0.5f);
	int maxX = int(sweep.p1.x+0.5f);
	int maxY = int(sweep.p1.y+0.5f);

	// union buckets for those sectors
	for(int x=minX; x<=maxX; ++x)
	for(int y=minY; y<=maxY; ++y) {
		outResult |= bucketFor(x,y);
	}

}

void CollisionSystem::debugDraw(LinePlotter* plotter) {
	unsigned slot;
	for(auto i=(~freeSlots).listBits(); i.next(slot);) {
		auto& c = slots[slot];
		plot(plotter, PIXELS_PER_METER * c.box.topLeft(), PIXELS_PER_METER * c.box.topRight(), rgb(0xffff00));
		plot(plotter, PIXELS_PER_METER * c.box.topRight(), PIXELS_PER_METER * c.box.bottomRight(), rgb(0xffff00));
		plot(plotter, PIXELS_PER_METER * c.box.bottomRight(), PIXELS_PER_METER * c.box.bottomLeft(), rgb(0xffff00));
		plot(plotter, PIXELS_PER_METER * c.box.bottomLeft(), PIXELS_PER_METER * c.box.topLeft(), rgb(0xffff00));
	}
}
