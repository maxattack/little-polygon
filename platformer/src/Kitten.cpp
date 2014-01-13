#include "Game.h"

#define HALF_WIDTH      0.25f
#define HALF_HEIGHT     0.25f

void Kitten::init(AssetBundle* assets, SpriteBatch *batch, CollisionSystem* collisions) {
	
	// init physics
	auto pos = assets->userdata("kitten.position")->as<vec2>() - vec(0,0.1f);
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
	sprite = createSprite(
		batch,
		affineTranslation(PIXELS_PER_METER * position()),
		assets->image("kitten")
	);
	setFlipped(sprite, true);

}
