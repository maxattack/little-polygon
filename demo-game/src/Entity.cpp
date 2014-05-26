#include "game.h"

Entity::Entity(Vec2 aPos, Vec2 aSize) :
position(aPos.x, aPos.y - 0.5f * aSize.y),
speed(0,0),
anchor(0, 0.5f * aSize.y),
halfSize(0.5f * aSize)
{
}

bool Entity::overlaps(const Entity *other) {
	auto dx = fabsf(position.x - other->position.x);
	auto dy = fabsf(position.y - other->position.y);
	return dx < halfSize.x + other->halfSize.x &&
		dy < halfSize.y + other->halfSize.y;
}

void Entity::move(int* hitX, int* hitY) {
	*hitX = 0;
	*hitY = 0;
	auto displacement = speed * lpTimer.dt();
	auto p1 = position + displacement;
	if (gWorld.mask.check(p1 - halfSize, p1 + halfSize)) {
		
		// COLLISION, RESOLVE AXES SEPARATELY
		if (displacement.y > kDeadZone) {
			// MOVE DOWN
			float dy;
			if (gWorld.mask.checkBottom(vec(left(), position.y), vec(right(), bottom()+displacement.y), &dy)) {
				position.y += std::max(displacement.y + dy, 0.0f);
				*hitY = 1;
				speed.y = 0.0f;
			} else {
				position.y += displacement.y;
			}
		} else if (displacement.y < -kDeadZone) {
			// MOVE UP
			float dy;
			if (gWorld.mask.checkTop(vec(left(), top() + displacement.y), vec(right(), position.y), &dy)) {
				position.y += std::min(displacement.y + dy, 0.0f);
				*hitY = -1;
				speed.y = 0.0f;
			} else {
				position.y += displacement.y;
			}
		}
		if (displacement.x > kDeadZone) {
			// MOVE RIGHT
			float dx;
			if (gWorld.mask.checkRight(vec(position.x, top()), vec(right() + displacement.x, bottom()), &dx)) {
				position.x += std::max(displacement.x + dx, 0.0f);
				*hitX = 1;
				speed.x = 0.0f;
			} else {
				position.x += displacement.x;
			}
		} else if (displacement.x < -kDeadZone) {
			// MOVE LEFT
			float dx;
			if (gWorld.mask.checkLeft(vec(left() + displacement.x, top()), vec(position.x, bottom()), &dx)) {
				position.x += std::min(displacement.x + dx, 0.0f);
				*hitX = -1;
				speed.x = 0.0f;
			} else {
				position.x += displacement.x;
			}
		}
	} else {
		// FREEFALL
		position = p1;
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
	lpLines.plotBox(position - halfSize, position + halfSize, rgb(0xffffff));
}