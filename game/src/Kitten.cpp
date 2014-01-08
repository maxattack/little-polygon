#include "Game.h"

#define HALF_WIDTH      0.25f
#define HALF_HEIGHT     0.25f

void initialize(Kitten& kitten, AssetBundle& assets, CollisionSystem& collisions) {
	
	// init fx
	kitten.image = assets.image("kitten");
	kitten.frame = 0;
	kitten.flipped = true;

	// init physics
	auto position = assets.userdata("kitten.position")->as<vec2>() - vec(0,0.1f);
	kitten.collider = collisions.addCollider(
		aabb(
			position-vec(HALF_WIDTH, 2*HALF_HEIGHT),
			position+vec(HALF_WIDTH, 0)
		), 
		KITTEN_BIT, 
		ENVIRONMENT_BIT, 
		0, 
		&kitten
	);
	collisions.move(kitten.collider, vec(0,0.2f));
}

void draw(Kitten& kitten, SpriteBatch& spriteBatch) {
	drawImageScaled(
		spriteBatch, 
		kitten.image, 
		PIXELS_PER_METER * kitten.position(), 
		kitten.flipped ? vec(-1,1) : vec(1,1), 
		kitten.frame
	);	
}