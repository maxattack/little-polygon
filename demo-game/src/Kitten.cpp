#include "game.h"


Kitten::Kitten() :
Entity(
	gAssets.userdata("kitten.position")->get<Vec2>() - vec(0, kSlop),
	vec(kKittenWidth, kKittenHeight)
),
img(gAssets.image("kitten")),
dir(1)
{
	startSentry();
}

void Kitten::startSentry() {
	animTime = 0.0f;

	auto x0 = floorToInt(position.x);
	auto y = floorToInt(position.y);
	// kinda a hack :P
	if (!gWorld.mask.isFloor(x0, y)) {
		if (gWorld.mask.isFloor(x0+1, y)) { ++x0; }
		else { --x0; }
		
	}
	auto x1 = x0;
	
	while(gWorld.mask.isFloor(x0-1, y)) { --x0; }
	while(gWorld.mask.isFloor(x1+1, y)) { ++x1; }
	
	if (x0 == x1) {
		timeout = -1.0f;
	} else {
		sentryLeft = x0 + halfSize.x + kMetersPerPixel;
		sentryRight = x1 + 1.0f - halfSize.x - kMetersPerPixel;
		timeout = kKittenPause;
	}
	
	status = Pausing;
}

void Kitten::pickup() {
	status = Carried;
	carryProgress = 0.0f;
	carryBasePosition = position;
	animTime = 1.0f;
}

void Kitten::shoot() {
	status = Shooting;
	position.y = gWorld.hero.position.y;
	speed = vec(kKittenShootSpeed * dir, 0);
}

void Kitten::tick() {
	switch(status) {
		case Pausing: tickPausing(); break;
		case Walking: tickWalking(); break;
		case Carried: tickCarried(); break;
		case Shooting: tickShooting(); break;
		case Falling: tickFalling(); break;
		default: break;
	}
}

void Kitten::tickPausing() {
	if (timeout > 0.0f) {
		timeout -= gTimer.deltaSeconds;
		if (timeout <= 0.0f) {
			dir = -dir;
			animTime = 1.0f;
			gAssets.sample("catturn")->play();
			status = Walking;
		}
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
			animTime = 0.0f;
		}
	} else {
		position.x -= dt * kKittenMoveSpeed;
		if (position.x < sentryLeft) {
			position.x = sentryLeft;
			timeout = kKittenPause;
			status = Pausing;
			animTime = 0.0f;
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

void Kitten::tickShooting() {
	// travel until you hit a tile of the side of the screen
	int hitX, hitY; move(&hitX, &hitY);
	if (hitX) {
		gAssets.sample("collide")->play();
		speed.x = -dir * kKittenCollisionKickback;
		speed.y = jumpImpulse(kKittenCollisionHeight);
		status = Falling;
		gWorld.camera.quake();
	}
}


void Kitten::tickFalling() {
	speed.y += kGravity * gTimer.deltaSeconds;
	int hitX, hitY; move(&hitX, &hitY);
	if (hitY > 0) {
		startSentry();
	}
}

void Kitten::draw() {
	gSprites.drawImage(
		img,
		AffineMatrix(vec(dir, 0), vec(0, 1), pixelPosition()),
		int(animTime) % 2,
		status == Shooting ? rgb(0xffffff) : rgba(0)
	);
}
