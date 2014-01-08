#include "Game.h"

#define HALF_WIDTH      0.225f
#define HALF_HEIGHT     0.6f
#define GRAVITY         80.0f
#define JUMP_HEIGHT     2.05f
#define MOVE_SPEED      4.0f
#define MOVE_EASING     0.15f
#define WALK_ANIM_RATE  5.0f
#define REST_THRESHOLD  0.075f

void initialize(Hero& hero, AssetBundle& assets, CollisionSystem& collisions) {
	
	// init physics
	auto position = assets.userdata("hero.position")->as<vec2>() - vec(0,0.1f);
	hero.collider = collisions.addCollider(
		aabb(
			position-vec(HALF_WIDTH, 2*HALF_HEIGHT),
			position+vec(HALF_WIDTH, 0)
		), 
		HERO_BIT, 
		ENVIRONMENT_BIT, 
		KITTEN_BIT, 
		&hero
	);
	auto collision = collisions.move(hero.collider, vec(0,0.2f));
	hero.speed = vec(0,0);
	hero.grounded = collision.hitBottom;

	// init fx
	hero.image = assets.image("hero");
	hero.framef = 0;
	hero.frame = 0;
	hero.flipped = 0;

	hero.sfxJump = assets.sample("jump");
	hero.sfxFootfall = assets.sample("footfall");

}

void tick(Hero& hero, PlayerInput& input, CollisionSystem& collisions, float dt) {

	// jumping and freefall
	if (hero.grounded && input.jumpPressed) {
		play(hero.sfxJump);
		hero.speed.y = - sqrtf(2.f * GRAVITY * JUMP_HEIGHT);
		hero.grounded = false;
	} else {
		hero.speed.y += GRAVITY * dt;
	}

	// running
	switch(input.xdir()) {
		case -1:
			hero.speed.x = easeTowards(hero.speed.x, -MOVE_SPEED, MOVE_EASING, dt);
			hero.flipped = true;
			break;
		case 1:
			hero.speed.x = easeTowards(hero.speed.x, MOVE_SPEED, MOVE_EASING, dt);
			hero.flipped = false;
			break;
		default:
			hero.speed.x = easeTowards(hero.speed.x, 0, MOVE_EASING, dt);
			break;

	}

	// resolving collisions
	auto result = collisions.move(hero.collider, hero.speed * dt);
	bool wasGrounded = hero.grounded;
	if ((hero.grounded = result.hitBottom)) {
		hero.speed.y = 0;
		if (!wasGrounded) {
			hero.frame = 0;
			hero.framef = 0;			
			play(hero.sfxFootfall);
		}
	}
	if (result.hitHorizontal) {
		hero.speed.x = 0;
	}

	// Trigger events[8];
	// int nTriggers = collisions.queryTriggers(hero.collider, arraysize(events), events);
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
 	if (hero.grounded) {
        auto sx = fabs(hero.speed.x);
        if (sx > REST_THRESHOLD) {
            hero.framef += WALK_ANIM_RATE * sx * dt;
            hero.framef = fmodf(hero.framef, 3.f);
            int fr = int(hero.framef);
            if (hero.frame != fr && fr == 2) {
            	play(hero.sfxFootfall);
            }
           	hero.frame = fr;
        } else {
            hero.framef = 0;
            hero.frame = 0;
        }
    } else {
        hero.framef = 0;
        hero.frame = 2;
    }    

}

void draw(Hero& hero, SpriteBatch& spriteBatch) {
	drawImageScaled(
		spriteBatch, 
		hero.image, 
		PIXELS_PER_METER * hero.position(), 
		hero.flipped ? vec(-1,1) : vec(1,1), 
		hero.frame
	);
}


