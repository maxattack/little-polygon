#include "Game.h"

#define HALF_WIDTH      0.25f
#define HALF_HEIGHT     0.25f

void Kitten::init(AssetBundle* assets, CollisionSystem* collisions) {
	
	// init fx
	image = assets->image("kitten");
	frame = 0;
	flipped = true;

	// init physics
	auto position = assets->userdata("kitten.position")->as<vec2>() - vec(0,0.1f);
	collider = collisions->addCollider(
		aabb(
			position-vec(HALF_WIDTH, 2*HALF_HEIGHT),
			position+vec(HALF_WIDTH, 0)
		), 
		KITTEN_BIT, 
		ENVIRONMENT_BIT, 
		0, 
		this
	);
	collisions->move(collider, vec(0,0.2f));
}

void Kitten::draw(SpriteBatch* spriteBatch) {
	drawImageScaled(
		spriteBatch, 
		image, 
		PIXELS_PER_METER * position(), 
		flipped ? vec(-1,1) : vec(1,1), 
		frame
	);	
}