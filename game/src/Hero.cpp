#include "GameWorld.h"

#define HALF_WIDTH      0.225f
#define HALF_HEIGHT     0.6f
#define GRAVITY         80.0f
#define JUMP_HEIGHT     2.05f
#define MOVE_SPEED      4.0f
#define MOVE_EASING     0.25f
#define WALK_ANIM_RATE  4.0f
#define REST_THRESHOLD  0.075f

GameWorld::Hero::Hero(GameWorld *aGame) : 
game(aGame), 
image(game->assets->image("hero")) {

	// init bounding box and snap to ground
	auto position = game->assets->userdata("hero.position")->as<vec2>() - vec(0,0.1f);
	collider = game->collisionSystem.addCollider(aabb(
		position-vec(HALF_WIDTH, 2*HALF_HEIGHT),
		position+vec(HALF_WIDTH, 0)
	), HERO_BIT, ENVIRONMENT_BIT, KITTEN_BIT, this);
	Collision result;
	game->collisionSystem.move(collider, vec(0,0.2f), &result);

}

void GameWorld::Hero::tick() {

	float dt = game->timer.scaledDeltaTime;

	// jumping and freefall
	if (grounded && game->jumpPressed) {
		game->assets->sample("jump")->play();
		speed.y = - sqrtf(2.f * GRAVITY * JUMP_HEIGHT);
		grounded = false;
	} else {
		speed.y += GRAVITY * dt;
	}

	// running
	auto kb = SDL_GetKeyboardState(0);
	if (kb[SDL_SCANCODE_LEFT]) {
		speed.x = easeTowards(speed.x, -MOVE_SPEED, MOVE_EASING, dt);
		flipped = true;
	} else if (kb[SDL_SCANCODE_RIGHT]) {
		speed.x = easeTowards(speed.x, MOVE_SPEED, MOVE_EASING, dt);
		flipped = false;
	} else {
		speed.x = 0;
	}

	// resolving collisions
	Collision result;
	game->collisionSystem.move(collider, speed * dt, &result);
	if ((grounded = result.hitBottom)) {
		speed.y = 0;
	}
	if (result.hitHorizontal) {
		speed.x = 0;
	}

	TriggerEvent events[8];
	int nTriggers = game->collisionSystem.resolveTriggers(collider, 8, events);
	for(int i=0; i<nTriggers; ++i) {
		switch(events[i].type) {
			case TRIGGER_EVENT_ENTER:
				LOG_MSG("ENTER");
				break;
			case TRIGGER_EVENT_STAY:
				break;
			case TRIGGER_EVENT_EXIT:
				LOG_MSG("EXIT");
				break;
		}
	}

	// update fx
 	if (grounded) {
        auto sx = fabs(speed.x);
        if (sx > REST_THRESHOLD) {
            framef += WALK_ANIM_RATE * sx * dt;
            framef = fmodf(framef, 3.f);
            int fr = int(framef);
            if (frame != fr && fr == 2) {
            	game->assets->sample("footfall")->play();
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

void GameWorld::Hero::draw() {
	auto p = PIXELS_PER_METER * collider->box.bottomCenter();
	game->batch.drawImageScaled(image, p, flipped ? vec(-1,1) : vec(1,1), frame);
}


