#include "Game.h"

#define HALF_WIDTH      0.225f
#define HALF_HEIGHT     0.6f
#define GRAVITY         80.0f
#define JUMP_HEIGHT     2.05f
#define MOVE_SPEED      4.0f
#define MOVE_EASING     0.15f
#define WALK_ANIM_RATE  5.0f
#define REST_THRESHOLD  0.075f

void Hero::init(AssetBundle* assets, CollisionSystem* collisions) {
	
	// init physics
	auto position = assets->userdata("hero.position")->as<vec2>() - vec(0,0.1f);
	collider = collisions->addCollider(
		aabb(
			position-vec(HALF_WIDTH, 2*HALF_HEIGHT),
			position+vec(HALF_WIDTH, 0)
		), 
		HERO_BIT, 
		ENVIRONMENT_BIT, 
		KITTEN_BIT, 
		this
	);
	auto collision = collisions->move(collider, vec(0,0.2f));
	speed = vec(0,0);
	grounded = collision.hitBottom;

	// init fx
	image = assets->image("hero");
	framef = 0;
	frame = 0;
	flipped = 0;

	sfxJump = assets->sample("jump");
	sfxFootfall = assets->sample("footfall");

}

void Hero::tick(PlayerInput* input, CollisionSystem* collisions, float dt) {

	// jumping and freefall
	if (grounded && input->jumpPressed) {
		play(sfxJump);
		speed.y = - sqrtf(2.f * GRAVITY * JUMP_HEIGHT);
		grounded = false;
	} else {
		speed.y += GRAVITY * dt;
	}

	// running
	switch(input->xdir()) {
		case -1:
			speed.x = easeTowards(speed.x, -MOVE_SPEED, MOVE_EASING, dt);
			flipped = true;
			break;
		case 1:
			speed.x = easeTowards(speed.x, MOVE_SPEED, MOVE_EASING, dt);
			flipped = false;
			break;
		default:
			speed.x = easeTowards(speed.x, 0, MOVE_EASING, dt);
			break;

	}

	// resolving collisions
	auto result = collisions->move(collider, speed * dt);
	bool wasGrounded = grounded;
	if ((grounded = result.hitBottom)) {
		speed.y = 0;
		if (!wasGrounded) {
			frame = 0;
			framef = 0;			
			play(sfxFootfall);
		}
	}
	if (result.hitHorizontal) {
		speed.x = 0;
	}

	// Trigger events[8];
	// int nTriggers = collisions->queryTriggers(collider, arraysize(events), events);
	// for(int i=0; i<nTriggers; ++i) {
	// 	switch(events[i].type) {
	// 		case Trigger::ENTER:
	// 			LOG_MSG("ENTER");
	// 			break;
	// 		case Trigger::EXIT:
	// 			LOG_MSG("EXIT");
	// 			break;
	// 	}
	// }

	// update fx
 	if (grounded) {
        auto sx = fabs(speed.x);
        if (sx > REST_THRESHOLD) {
            framef += WALK_ANIM_RATE * sx * dt;
            framef = fmodf(framef, 3.f);
            int fr = int(framef);
            if (frame != fr && fr == 2) {
            	play(sfxFootfall);
            }
           	frame = fr;
        } else {
            framef = 0;
            frame = 0;
        }
    } else {
        framef = 0;
        frame = 2;
    }    

}

void Hero::draw(SpriteBatch* spriteBatch) {
	drawImageScaled(
		spriteBatch, 
		image, 
		PIXELS_PER_METER * position(), 
		flipped ? vec(-1,1) : vec(1,1), 
		frame
	);
}

