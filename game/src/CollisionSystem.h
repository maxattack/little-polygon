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
#include <littlepolygon_utils.h>
#include <littlepolygon_templates.h>


//------------------------------------------------------------------------------
// CONSTANTS

#define PIXELS_PER_METER 16
#define METERS_PER_PIXEL (1.0f/16.0f)

#define COLLIDER_CAPACITY 1024
#define CONTACT_CAPACITY  1024	
#define BUCKET_COUNT      1024

//------------------------------------------------------------------------------
// HELPERS

struct AABB {
	vec2 p0; // min extent
	vec2 p1; // max extent

	inline vec2 center() const { return 0.5f * (p0 + p1); }
	inline vec2 size() const { return p1 - p0; }
	
	inline vec2 topLeft() const { return p0; }
	inline vec2 topRight() const { return vec(p1.x, p0.y); }
	inline vec2 bottomLeft() const { return vec(p0.x, p1.y); }
	inline vec2 bottomRight() const { return p1; }
	inline vec2 bottomCenter() const { return vec(0.5f*(p0.x+p1.x), p1.y); }

	inline float left() const { return p0.x; }
	inline float right() const { return p1.x; }
	inline float top() const { return p0.y; }
	inline float bottom() const { return p1.y; }

	inline bool overlaps(const AABB& box) const {
		return 
			p0.x < box.p1.x && p1.x > box.p0.x &&
			p0.y < box.p1.y && p1.y > box.p0.y ;
	}
};

inline AABB aabb(float x0, float y0, float x1, float y1) {
	AABB result = { vec(x0,y0), vec(x1,y1) };
	return result;
}

inline AABB aabb(vec2 p0, vec2 p1) {
	AABB result = { p0, p1 };
	return result;
}

//------------------------------------------------------------------------------

struct Collider {
	AABB box;
	uint32_t categoryMask;
	uint32_t collisionMask;
	uint32_t triggerMask;
	void *userData;

	inline bool collides(const Collider *c) const {
		return (collisionMask & c->categoryMask) &&
		       box.overlaps(c->box);
	}

	inline bool triggers(const Collider *c) const {
		return (triggerMask & c->categoryMask) &&
		       box.overlaps(c->box);
	}
};

//------------------------------------------------------------------------------

struct Collision {
	union {                      // collision directions (since everything is
		uint32_t hit;            // an AABB we can make this simplification)
		struct {
			uint8_t hitBottom;
			uint8_t hitTop;
			uint8_t hitLeft;
			uint8_t hitRight;
		};
		struct {
			uint16_t hitVertical;
			uint16_t hitHorizontal;
		};
	};
};


//------------------------------------------------------------------------------

struct Trigger {
	enum TriggerType { ENTER, EXIT };

	TriggerType type;
	Collider* trigger;
};

//------------------------------------------------------------------------------

class CollisionSystem {
public:

	CollisionSystem();

	Collider* addCollider(
		const AABB& box, 
		uint32_t categoryMask = 0xffffffff, 
		uint32_t collisionMask = 0xffffffff,
		uint32_t triggerMask = 0x00000000,
		void *userData = 0
	);
	void removeCollider(Collider *collider);
		
	Collision move(Collider *c, vec2 offset);
	int queryTriggers(Collider *c, int outCapacity, Trigger *resultBuf);
	int queryColliders(const AABB& box, uint32_t mask, int outCapacity, Collider **resultBuf);

	void debugDraw(LinePlotter* plotter);

private:
	typedef Bitset<COLLIDER_CAPACITY> ColliderSet;

	ColliderSet freeSlots;
	Collider slots[COLLIDER_CAPACITY];
	ColliderSet buckets[BUCKET_COUNT];

	struct Contact {
		Collider *collider;
		Collider *trigger;
	};

	uint32_t nContacts = 0;
	Contact contacts[CONTACT_CAPACITY];

	ColliderSet& bucketFor(int x, int y);
	void hash(Collider *c);
	void unhash(Collider *c);
	void broadPhase(const AABB& sweep, ColliderSet& outResult);

};





