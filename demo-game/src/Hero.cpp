#include "game.h"



Hero::Hero() :
Entity(
	gAssets.userdata("hero.position")->get<vec2>() - vec(0, 1),
	vec(kHeroWidth, kHeroHeight)
),
img(gAssets.image("hero"))
{
}

void Hero::tick() {

	*speed() = kHeroMoveSpeed * gWorld.input.dir();
	
//	// MOVEMENT
//	// TODO: EASING
//	speed()->x = kHeroMoveSpeed * gWorld.input.dirX();
//	
//	// GRAVITY
//	speed()->y += kGravity * gTimer.deltaSeconds;
//	
//	// JUMPING
//	if (contactBottom() && gWorld.input.pressedJump()) {
//		speed()->y = -kHeroJumpSpeed;
//	}
	
	move();
}

void Hero::draw() {
	gSprites.drawImage(img, pixelPosition());
}

