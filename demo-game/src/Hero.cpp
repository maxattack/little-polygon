#include "game.h"



Hero::Hero() :
Entity(
	gAssets.userdata("hero.position")->get<vec2>() - vec(0, 1),
	vec(kHeroWidth, kHeroHeight)
),
img(gAssets.image("hero")),
dir(1),
animTime(0.0f), runningTime(0.0f), yScale(1.0f)
{
}

void Hero::tick() {
	
	float dt = gTimer.deltaSeconds;
	
	// MOVEMENT
	auto speedTarget = kHeroMoveSpeed * gWorld.input.dirX();
	auto easing = gWorld.input.dirX() == 0 ? 0.2f : 0.4f;
	pspeed()->x = easeTowards(speedX(), speedTarget, easing, dt);
	
	if (gWorld.input.dirX()) { dir = gWorld.input.dirX(); }
	
	// GRAVITY
	pspeed()->y += kGravity * dt;
	
	// JUMPING
	if (contactBottom() && gWorld.input.pressedJump()) {
		gAssets.sample("jump")->play();
		pspeed()->y = -sqrtf(2.0f * kHeroJumpHeight * kGravity);
	}
	
	auto wasGrounded = contactBottom();
	
	move();
	
	if (contactBottom()) {
		if (!wasGrounded) {
			// LANDING FX
			animTime = 0.0f;
			runningTime = 0.0f;
			yScale = 0.8f;
			gAssets.sample("land")->play();
		} else {
			
			// RUNNING FX
			animTime += kHeroStepsPerMeter * dt * abs(speedX());

			// EASE TO NORMAL SCALE
			yScale = easeTowards(yScale, 1.0f, 0.2f, dt);
		}
	} else {
		
		// EASE TO FREEFALL SCALE
		yScale = easeTowards(yScale, 1.025f, 0.2f, dt);
	}
	
	
	
}

void Hero::draw() {
	gSprites.drawImage(
		img,
		AffineMatrix(vec(dir,0), vec(0,yScale), pixelPosition()),
		getFrame()
	);
}

bool Hero::isStandingStill() const {
	return speedX() > -0.133f && speedX() < 0.133f;
}

int Hero::getFrame() const {
	if (!contactBottom()) {
		return 1;
	} else if (!isStandingStill()) {
		return int(animTime) % 2;
	} else {
		return 0;
	}
}
