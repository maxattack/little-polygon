#pragma once
#include "config.h"

//------------------------------------------------------------------------------
// CONSTANTS

#define COLLIDER_CAPACITY 128
#define CONTACT_CAPACITY  128

enum TriggerEventType {
	TRIGGER_EVENT_ENTER,
	TRIGGER_EVENT_STAY,
	TRIGGER_EVENT_EXIT
};

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

struct TriggerEvent {
	TriggerEventType type;
	ID trigger;
};

struct Collision {
	union {
		uint32_t hit;
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

	int triggerEventCount;
	TriggerEvent *triggerEvents;

	vec2 offset;

};

//------------------------------------------------------------------------------
// MAIN INTERFACE

class CollisionSystem {
public:
	CollisionSystem();

	ID addCollider(const AABB& box, 
	               uint32_t categoryMask=0xffffffff, 
	               uint32_t collisionMask=0xffffffff,
	               uint32_t triggerMask=0,
	               void *userData=0);
	
	bool move(ID id, vec2 offset, Collision *outResult);
	
	bool move(ID id, vec2 offset) {
		Collision result;
		return move(id, offset, &result);
	}
	
	void removeCollider(ID id);

	inline AABB& bounds(ID id) { return colliders[id].box; }
	inline uint32_t& categoryMask(ID id) { return colliders[id].categoryMask; }
	inline uint32_t& collisionMask(ID id) { return colliders[id].collisionMask; }
	inline void*& userData(ID id) { return userDataBuf[POOL_SLOT_INDEX(id)]; }

	void debugDraw(LinePlotter& plotter);

private:
	
	struct Collider {
		ID id;
		uint32_t categoryMask;
		uint32_t collisionMask;
		uint32_t triggerMask;
		AABB box;

		inline bool collides(const Collider &c) {
			return id != c.id && 
			       (collisionMask & c.categoryMask) &&
			       box.overlaps(c.box);
		}

		inline bool triggers(const Collider &c) {
			return id != c.id &&
			       (triggerMask & c.categoryMask) &&
			       box.overlaps(c.box);
		}
	};

	struct Contact {
		ID collider;
		ID trigger;
	};

	int ncontacts = 0;

	Pool<Collider, COLLIDER_CAPACITY> colliders;

	Contact contacts[CONTACT_CAPACITY];
	TriggerEvent triggerEventBuf[CONTACT_CAPACITY];

	void* userDataBuf[COLLIDER_CAPACITY];

	int findContact(ID collider, ID trigger);
};
