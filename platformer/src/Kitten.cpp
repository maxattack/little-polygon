#include "platformer.h"

#define HALF_WIDTH      0.25f
#define HALF_HEIGHT     0.25f

void Kitten::init(AssetRef assets, SpriteBatchRef batch, CollisionSystem* collisions) {
	
	// init physics
	auto pos = assets.userdata("kitten.position")->as<vec2>() - vec(0,0.1f);
	collider = collisions->addCollider(
		aabb(
			pos-vec(HALF_WIDTH, 2*HALF_HEIGHT),
			pos+vec(HALF_WIDTH, 0)
		), 
		KITTEN_BIT, 
		ENVIRONMENT_BIT, 
		0, 
		this
	);
	collisions->move(collider, vec(0,0.2f));

	// init fx
	xform = AffineMatrix(vec(-1,0), vec(0,1), PIXELS_PER_METER * position());
	sprite = batch.addSprite(assets.image("kitten"), &xform);

}
