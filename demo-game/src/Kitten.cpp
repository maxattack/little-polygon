#include "game.h"


Kitten::Kitten(const WorldData& data) :
Entity(
	data.kittenPosition - vec(0, kSlop),
	vec(kKittenWidth, kKittenHeight)
),
img(lpAssets.image("kitten")),
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
		timeout -= lpTimer.deltaSeconds;
		if (timeout <= 0.0f) {
			dir = -dir;
			animTime = 1.0f;
			lpAssets.sample("catturn")->play();
			status = Walking;
		}
	}
}

void Kitten::tickWalking() {
	float dt = lpTimer.deltaSeconds;
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
		carryProgress += lpTimer.deltaSeconds / kKittenPickupTime;
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
		lpAssets.sample("collide")->play();
		speed.x = -dir * kKittenCollisionKickback;
		speed.y = jumpImpulse(kKittenCollisionHeight);
		status = Falling;
		gWorld.camera.quake();
		int tileX = floorToInt(position.x + 0.5f * hitX);
		int tileY = floorToInt(position.y);
		gWorld.destroyTile(tileX, tileY)       ||
			gWorld.destroyTile(tileX, tileY+1) ||
			gWorld.destroyTile(tileX, tileY-1);
	}
}


void Kitten::tickFalling() {
	speed.y += kGravity * lpTimer.deltaSeconds;
	int hitX, hitY; move(&hitX, &hitY);
	if (hitY > 0) {
		startSentry();
	}
}

void Kitten::draw() {
	lpSprites.drawImage(
		img,
		AffineMatrix(vec(dir, 0), vec(0, 1), pixelPosition()),
		int(animTime) % 2,
		status == Shooting ? rgb(0xffffff) : rgba(0)
	);
}
