#include "platformer.h"

#define HALF_WIDTH      0.25f
#define HALF_HEIGHT     0.25f

void Kitten::init(AssetRef assets, SpriteBatchRef batch, CollisionSystemRef collisions) {
	
	// init physics
	xform = affineScale(vec(-1,1));
	auto pos = assets.userdata("kitten.position")->as<vec2>() - vec(0,0.1f);
	collider = collisions.addCollider(
		aabb(pos-vec(HALF_WIDTH, 2*HALF_HEIGHT),
		     pos+vec(HALF_WIDTH, 0)), 
		KITTEN_BIT, ENVIRONMENT_BIT, 0, true, this
	);
	collider.setDelegate(&xform, vec(HALF_WIDTH, 2*HALF_HEIGHT));
	collider.move(vec(0,0.2f));

	// init fx
	sprite = batch.addSprite(assets.image("kitten"), &xform);

}
