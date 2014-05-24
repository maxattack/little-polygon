#include "game.h"

Entity::Entity(vec2 aPos, vec2 aSize) :
mPosition(aPos.x, aPos.y - 0.5f * aSize.y),
mSpeed(0,0),
mOffset(0, 0.5f * aSize.y),
mHalfSize(0.5f * aSize),
hitX(0), hitY(0)
{
}

void Entity::move() {
	hitX = 0;
	hitY = 0;
	auto displacement = mSpeed * gTimer.deltaSeconds;
	auto p1 = mPosition + displacement;
	if (gWorld.mask.check(p1 - mHalfSize, p1 + mHalfSize)) {
		
		// COLLISION, RESOLVE AXES SEPARATELY
		if (displacement.y > kDeadZone) {
			// MOVE DOWN
			float dy;
			if (gWorld.mask.checkBottom(vec(left(), bottom()+displacement.y), vec(right(), mPosition.y), &dy)) {
				mPosition.y += std::max(displacement.y + dy, 0.0f);
				hitY = 1;
				mSpeed.y = 0.0f;
			} else {
				mPosition.y += displacement.y;
			}
		} else if (displacement.y < -kDeadZone) {
			// MOVE UP
			float dy;
			if (gWorld.mask.checkTop(vec(left(), mPosition.y), vec(right(), top() + displacement.y), &dy)) {
				mPosition.y += std::min(displacement.y + dy, 0.0f);
				hitY = -1;
				mSpeed.y = 0.0f;
			} else {
				mPosition.y += displacement.y;
			}
		}
		if (displacement.x > kDeadZone) {
			// MOVE RIGHT
			float dx;
			if (gWorld.mask.checkRight(vec(mPosition.x, bottom()), vec(right() + displacement.x, top()), &dx)) {
				mPosition.x += std::max(displacement.x + dx, 0.0f);
				hitX = 1;
				mSpeed.x = 0.0f;
			} else {
				mPosition.x += displacement.x;
			}
		} else if (displacement.x < -kDeadZone) {
			// MOVE LEFT
			float dx;
			if (gWorld.mask.checkLeft(vec(left() + displacement.x, bottom()), vec(mPosition.x, top()), &dx)) {
				mPosition.x += std::min(displacement.x + dx, 0.0f);
				hitX = -1;
				mSpeed.x = 0.0f;
			} else {
				mPosition.x += displacement.x;
			}
		}
	} else {
		// FREEFALL
		mPosition = p1;
	}
}

void Entity::debugDraw() {
//	int x0 = left();
//	int x1 = right();
//	int y0 = top();
//	int y1 = bottom();
//	for(int x=x0; x<=x1; ++x)
//	for(int y=y0; y<=y1; ++y) {
//		gLines.plotBox(vec(x,y), vec(x+1, y+1), rgb(0xaaaaaa));
//	}
	gLines.plotBox(mPosition - mHalfSize, mPosition + mHalfSize, rgb(0xffffff));
}