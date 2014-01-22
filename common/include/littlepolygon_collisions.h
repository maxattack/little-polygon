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

#include "littlepolygon_math.h"
#include "littlepolygon_graphics.h"
#include "littlepolygon_fk.h"
#include "littlepolygon_bitset.h"

// A very simple bounding-box collision system, useable for simple 2D
// games like platformers and top-down adventure games.  This is not a
// fully dynamic physics system; it just lets you detect collisions and
// obey non-overlapping constraints.

// For dynamics, it could be used *in conjunction* with something like
// chipmunk or Box2D.

// Collisions are determined using a simple broad-phase algorithm.  The
// scene is partitioned into 1mX1m grid "sectors" which are hashed to
// a fixed number of "buckets".  When a box is added to the spatial hash
// it is flagged in each bucket that corresponds to a sector that it
// overlaps.

// The choice of what "one meter" means is context dependent, but in general
// for tile-based games it should be a tile width, and for platformers it
// should be about the height of the hero's avatar.  Physical coordinates
// are converted using a metersToDisplay matrix.

// Wishlist:
//   - one-way "platform"/"portal" line-colliders
//   - b2Body delegate

struct CollisionContext;
struct Collider;
struct TriggerEvent;
class CollisionSystemRef;
class ColliderRef;
class ColliderDelegate;
class QueryIterator;

typedef Bitset<1024> ColliderSet;

//------------------------------------------------------------------------------
// Axis-Aligned Bounding Box POD Type
//------------------------------------------------------------------------------

struct AABB {
	vec2 p0; // min extent
	vec2 p1; // max extent

	AABB() {}
	AABB(float x0, float y0, float x1, float y1) : p0(x0,y0), p1(x1,y1) {}
	AABB(vec2 a0, vec2 a1) : p0(a0), p1(a1) {}

	inline bool valid() const { return p0.x <= p1.x && p0.y <= p1.y; }

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

	inline bool contains(vec2 p) {
		return p0.x <= p.x && p1.x >= p.x &&
			p0.y <= p.y && p1.y >= p.y;
	}

	inline bool overlaps(const AABB& box) const {
		return 
			p0.x < box.p1.x && p1.x > box.p0.x &&
			p0.y < box.p1.y && p1.y > box.p0.y ;
	}
};


//------------------------------------------------------------------------------
// RAY TYPE
//------------------------------------------------------------------------------

struct Ray {
	vec2 p0;
	vec2 p1;

	Ray() {}
	Ray(float x0, float y0, float x1, float y1) : p0(x0,y0), p1(x1,y1) {}
	Ray(vec2 a0, vec2 a1) : p0(a0), p1(a1) {}

	vec2 offset() const { return p1 - p0; }
	vec2 pointAt(float u) const { return p0 + u * (p1 - p0); }

	// u >= 0  <-- intersection at p0 + u * (p1 - p0)
	// u < -1  <-- no intersection
	float intersect(const AABB& box) const;
};

//------------------------------------------------------------------------------
// COLLISION RESULTS
//------------------------------------------------------------------------------

union Collision {
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

	Collision() {}
	Collision(uint32_t mask) : hit(mask) {}

	Collision operator|(const Collision c) { return hit | c.hit; }
	Collision& operator|=(const Collision c) { hit |= c.hit; return *this; }
};

enum TriggerType { ENTER, STAY, EXIT };

//------------------------------------------------------------------------------
// MAIN INTERFACE
//------------------------------------------------------------------------------

CollisionContext *createCollisionSystem(
	size_t colliderCapacity=1024, 
	size_t numBuckets=1024, 
	size_t maxContacts=1024
);

class CollisionSystemRef {
private:
	CollisionContext* context;

public:
	CollisionSystemRef() {}
	CollisionSystemRef(CollisionContext *aContext) : context(aContext) {}

	operator CollisionContext*() { return context; }
	operator bool() const { return context; }

	void destroy();

	// Set the unit conversion from physics to display
	const AffineMatrix& metersToDisplay() const;
	void setMetersToDisplay(const AffineMatrix& matrix);
	inline void setMetersToDisplay(float k) { 
		setMetersToDisplay(affineScale(vec(k,k))); 
	}

	Collider *addCollider(
		const AABB& box, 
		uint32_t categoryMask  = 0xffffffff, 
		uint32_t collisionMask = 0xffffffff,
		uint32_t triggerMask   = 0x00000000,
		bool enabled = true,
		void *userData = 0
	);

	// u >= 0 := hit
	// u < 0  := miss
	float raycast(const Ray& ray, uint32_t mask, ColliderRef *result);

	// Will use the metersToDisplay matrix for plotting
	void debugDraw(LinePlotterRef plotter, Color c);
};

class ColliderDelegate {
public:
	virtual void setPosition(vec2 position) = 0;
};

class ColliderRef {
private:
	Collider *collider;

public:
	ColliderRef() {}
	ColliderRef(Collider *aCollider) : collider(aCollider) {}

	operator Collider*() { return collider; }
	operator bool() const { return collider; }

	void destroy();

	CollisionSystemRef context();

	// Try to move by offset.  May be clipped to preserve overlapping constraints,
	// in which case the result will reflect on which side this happens.  Constraints
	// are solved for the X/Y axis separately, so you will still fit "snuggly" into 
	// corners.  Very large moves may not be handled gracefully, and one should consider
	// breaking these into sub-moves and "or"ing the collisions results together.
	Collision move(vec2 offset);

	// warning: this method may put the box into an inconsistent
	// state; use with caution :P
	void setPosition(vec2 topLeft);

	// Find all triggers and compute deltas (ENTER, STAY, EXIT) based on the 
	// *last call to queryTriggers*, therefore you should call this every tick, 
	// even if you're not handling the events.
	typedef void (*TriggerEventCallback)(TriggerType, ColliderRef);
	
	void queryTriggers(TriggerEventCallback cb=0);
	

	// Retrieve bounds in world-meters space
	AABB box() const;

	// The category mask identifies the "type" of the collider
	uint32_t categoryMask() const;

	// The collision mask identifies which types of colliders we should
	// not overlap (may not be symmetric!)
	uint32_t collisionMask() const;

	// The trigger mask identifies which types of colliders we are
	// interested in querying triggers for -- needs to be disjoint
	// with the collision mask (since these are more like "sensors")
	uint32_t triggerMask() const;

	// Offset, in meters, of the delegate
	vec2 pivot() const;

	// Optional attached data
	void *userData() const;

	template<typename T>
	T* get() const { return (T*)userData(); }

	// Colliders are enabled and disabled by adding/removing them
	// from the spatial hash, so they will no longer be included
	// in queries.
	void enable();
	void disable();

	// when a collider is 'bound' to an xform, it's translation
	// vector will be written to as a side-effect of move().
	// This result is first transformed by the context's
	// metersToDisplay matrix.
	// The optional pivot lets you choose a point other than the 
	// top-left corner.  This is expressed in meters.
	void setDelegate(AffineMatrix *xform, vec2 pivot=vec(0,0));
	void setDelegate(FkNodeRef node, vec2 pivot=vec(0,0));
	void setDelegate(ColliderDelegate *delegate, vec2 pivot=vec(0,0));
	void clearDelegate();

	void setCategoryMask(uint32_t mask);
	void setCollisionMask(uint32_t mask);
	void setTriggerMask(uint32_t mask);
	void setPivot(vec2 u);
	void setUserData(void *data);

	template<typename T>
	void set(T* data) { setUserData((void*)data); }

	// Quick polling methods for 1:1 interactions.  For 1:N interactions,
	// use the QueryIterator to utilize the spatial hash for filtering.
	bool collides(const ColliderRef other) const;
	bool triggers(const ColliderRef other) const;

};

//------------------------------------------------------------------------------
// ITERATORS
//------------------------------------------------------------------------------

class QueryIterator {
private:
	CollisionContext* context;
	AABB box;
	uint32_t mask;
	ColliderSet candidates;

public:
	QueryIterator(CollisionContext *context, const AABB& box, uint32_t mask);
	bool next(ColliderRef& next);
};
