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

#include "littlepolygon_go.h"
#include "littlepolygon_pools.h"
#include <Box2D/Box2D.h>

class BodyComponent : GoComponent {
	b2Body *body;

	int init();
	int enable();
	int disable();
	int destroy();
};

class PhysicsSystem {
public:

	// The physics->display matrix converts the physics coordinate system
	// to the display coordinates (meters->pixels).  Because the b2World is
	// not sampled directly in rendering, it can in theory be Step()ed on a 
	// secondary thread (assuming that the collision callbacks don't have 
	// side-effects)
	PhysicsSystem(b2World *aWorld, const AffineMatrix &aPhysicsToDisplay) : 
		world(aWorld), physicsToDisplay(aPhysicsToDisplay) {}

	BodyComponent *addBody(GameObjectRef go, const b2BodyDef& params);

	// b2Body transforms are copied to the FK heirarchy using setWorld(), so
	// it's generally best to attach physics bodies to root nodes.  Node xforms
	// are blindly overwritten, so if you need feedback from a script or something
	// it should be set on the body unless it's just some prerender fixup or something.
	void applyToNodes();

private:

	AffineMatrix physicsToDisplay;
	b2World *world;
	BitsetPool<BodyComponent> pool;
};

// TODO: Collider components?
