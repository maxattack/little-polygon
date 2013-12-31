#pragma once
#include "config.h"

#define COLLISION_SLOP 0.0001f

struct AABB {
	vec2 p0; // min extent
	vec2 p1; // max extent

	inline vec2 center() const { return 0.5f * (p0 + p1); }
	inline vec2 size() const { return p1 - p0; }
	
	// pixel positions
	inline vec2 topLeft() const { return p0; }
	inline vec2 topRight() const { return vec(p1.x, p0.y); }
	inline vec2 bottomLeft() const { return vec(p0.x, p1.y); }
	inline vec2 bottomRight() const { return p1; }

	inline float left() const { return p0.x; }
	inline float right() const { return p1.x; }
	inline float top() const { return p0.y; }
	inline float bottom() const { return p1.y; }

	inline bool overlaps(const AABB& box) const {
		return 
			p0.x + COLLISION_SLOP < box.p1.x && p1.x - COLLISION_SLOP > box.p0.x &&
			p0.y + COLLISION_SLOP < box.p1.y && p1.y - COLLISION_SLOP > box.p0.y ;
	}
};

inline AABB aabb(vec2 p0, vec2 p1) {
	AABB result = { p0, p1 };
	return result;
}

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
	vec2 offset;
};

struct Collider {
	ID id;
	uint32_t categoryMask;
	uint32_t collisionMask;
	AABB box;

	inline bool collides(const Collider &c) {
		return id != c.id && 
		       (collisionMask & c.categoryMask) &&
		       box.overlaps(c.box);
	}
};

class CollisionSystem {
public:
	CollisionSystem();

	ID addCollider(const AABB& box, uint32_t categoryMask=0xffffffff, uint32_t collisionMask=0xffffffff);
	bool move(ID id, vec2 offset, Collision *outResult);
	
	bool move(ID id, vec2 offset) {
		Collision result;
		return move(id, offset, &result);
	}
	
	void removeCollider(ID id);

	inline AABB& bounds(ID id) { return colliders[id].box; }
	inline uint32_t& categoryMask(ID id) { return colliders[id].categoryMask; }
	inline uint32_t& collisionMask(ID id) { return colliders[id].collisionMask; }

	void debugDraw(LinePlotter& plotter);

private:

	Pool<Collider, 128> colliders;

};
