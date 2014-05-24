#include "game.h"



Hero::Hero() :
Entity(
	gAssets.userdata("hero.position")->get<vec2>() - vec(0, 0.01f),
	vec(kHeroWidth, kHeroHeight)
),
img(gAssets.image("hero")),
dir(1),
animTime(0.0f), yScale(1.0f),
grounded(true)
{
	auto mus = Mix_LoadMUS("song.mid");
	if (mus) {
		Mix_PlayMusic(mus, -1);
	}
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
	if (grounded && gWorld.input.pressedJump()) {
		gAssets.sample("jump")->play();
		pspeed()->y = -sqrtf(2.0f * kHeroJumpHeight * kGravity);
	}
	
	auto wasGrounded = grounded;
	
	int hitX, hitY; move(&hitX, &hitY);
	
	grounded = hitY > 0;
	if (grounded) {
		if (!wasGrounded) {
			// LANDING FX
			animTime = 0.0f;
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
	if (!grounded) {
		return 1;
	} else if (!isStandingStill()) {
		return int(animTime) % 2;
	} else {
		return 0;
	}
}
