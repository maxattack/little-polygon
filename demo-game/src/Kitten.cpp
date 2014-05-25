#include "game.h"


Kitten::Kitten() :
Entity(
	gAssets.userdata("kitten.position")->get<vec2>() - vec(0, kSlop),
	vec(kKittenWidth, kKittenHeight)
),
img(gAssets.image("kitten")),
status(Pausing),
dir(1),
timeout(0.0f), animTime(0.0f)
{
	auto x0 = floorToInt(position.x);
	auto x1 = x0;
	auto y = floorToInt(position.y);
	
	while(gWorld.mask.isFloor(x0-1, y)) { --x0; }
	while(gWorld.mask.isFloor(x1+1, y)) { ++x1; }
	
	sentryLeft = x0 + halfSize.x + kMetersPerPixel;
	sentryRight = x1 + 1.0f - halfSize.x - kMetersPerPixel;
}

void Kitten::pickup() {
	status = Carried;
	carryProgress = 0.0f;
	carryBasePosition = position;
	animTime = 1.1f;
}

void Kitten::tick() {
	switch(status) {
		case Pausing: tickPausing(); break;
		case Walking: tickWalking(); break;
		case Carried: tickCarried(); break;
		default: break;
	}
}

void Kitten::tickPausing() {
	animTime = 0.0f;
	timeout -= gTimer.deltaSeconds;
	if (timeout < 0.0f) {
		dir = -dir;
		animTime = 1.0f;
		status = Walking;
	}
}

void Kitten::tickWalking() {
	float dt = gTimer.deltaSeconds;
	animTime += kKittenStepsPerMeter * dt;
	if (dir > 0) {
		position.x += dt * kKittenMoveSpeed;
		if (position.x > sentryRight) {
			position.x = sentryRight;
			timeout = kKittenPause;
			status = Pausing;
		}
	} else {
		position.x -= dt * kKittenMoveSpeed;
		if (position.x < sentryLeft) {
			position.x = sentryLeft;
			timeout = kKittenPause;
			status = Pausing;
		}
	}
}

void Kitten::tickCarried() {
	if (carryProgress < 1.0f) {
		carryProgress += gTimer.deltaSeconds / kKittenPickupTime;
		if (carryProgress > 1.0f) { carryProgress = 1.0f; }
		position = lerp(carryBasePosition, gWorld.hero.carryAnchor(), carryProgress) +
			vec(0, -parabola(carryProgress));
	} else {
		position = gWorld.hero.carryAnchor();
	}
	dir = gWorld.hero.carryDirection();
	
}

void Kitten::draw() {
	gSprites.drawImage(
		img,
		AffineMatrix(vec(dir, 0), vec(0, 1), pixelPosition()),
		int(animTime) % 2
	);
}
