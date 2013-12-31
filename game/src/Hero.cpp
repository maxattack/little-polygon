#include "game.h"

#define HALF_WIDTH      0.25f
#define HALF_HEIGHT     0.666f
#define GRAVITY         80.0f
#define JUMP_HEIGHT     1.5f
#define MOVE_SPEED      4.0f
#define MOVE_EASING     0.25f
#define WALK_ANIM_RATE  0.065f
#define REST_THRESHOLD  0.075f

void Hero::init(GameWorld *game) {

	// init physics
	auto position = game->assets->userdata("start")->get<vec2>();
	collider = game->collisionSystem.addCollider(aabb(
		position-vec(HALF_WIDTH, 2*HALF_HEIGHT),
		position+vec(HALF_WIDTH, 0)
	));
	speed = vec(0,0);
	grounded = false;

	// init fx
	image = game->assets->image("hero");
	frame = 0;
	framef = 0;
	flipped = false;

}

void Hero::tick(GameWorld *game) {

	float dt = game->timer.deltaSeconds();
	auto kb = SDL_GetKeyboardState(0);

	if (grounded && game->jumpPressed) {
		game->assets->sample("jump")->play();
		speed.y = - sqrtf( 2 * GRAVITY * JUMP_HEIGHT );
		grounded = false;
	} else {
		speed.y += GRAVITY * dt;
	}

	if (kb[SDL_SCANCODE_LEFT]) {
		speed.x = easeTowards(speed.x, -MOVE_SPEED, MOVE_EASING, dt);
		flipped = true;
	} else if (kb[SDL_SCANCODE_RIGHT]) {
		speed.x = easeTowards(speed.x, MOVE_SPEED, MOVE_EASING, dt);
		flipped = false;
	} else {
		speed.x = 0;
	}

	grounded = false;

	Collision result;
	game->collisionSystem.move(collider, speed * dt, &result);
	if (result.hitBottom) {
		grounded = true;
		speed.y = 0;
	}
	if (result.hitHorizontal) {
		speed.x = 0;
	}
}

void Hero::draw(GameWorld *game) {
 	if (grounded) {
        auto sx = fabs(speed.x);
        if (sx > REST_THRESHOLD) {
            framef += WALK_ANIM_RATE * sx;
            framef = fmodf(framef, 3.f);
            int fr = int(framef);
            if (frame != fr && fr == 0) {
            	game->assets->sample("footfall")->play();
            }
           	frame = fr;
        } else {
            framef = 0;
            if (frame != 0) {
            	frame = 0;
            	game->assets->sample("footfall")->play();
            }
        }
    } else {
        framef = 0;
        frame = 2;
    }                

	auto box = game->collisionSystem.bounds(collider);
	auto p = PIXELS_PER_METER * box.topLeft();
	p.x += PIXELS_PER_METER * HALF_WIDTH;
	p.y += 2 * PIXELS_PER_METER * HALF_HEIGHT;
	game->batch.drawImageScaled(image, p, flipped ? vec(-1,1) : vec(1,1), frame);
}


