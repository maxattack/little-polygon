#include "game.h"

Entity::Entity(vec2 aPos, vec2 aSize) :
mPosition(aPos),
mSpeed(0,0),
mLocalCenter(0, -0.5f * aSize.y),
mHalfSize(0.5f * aSize)
{
}

void Entity::tick(const TileMask *mask) {
	// TODO: KINEMATICS
}

void Entity::debugDraw() {
	auto center = mPosition + mLocalCenter;
	gLines.plotBox(center - mHalfSize, center + mHalfSize, rgb(0xffffff));
}