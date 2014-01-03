#include "GameWorld.h"

#define HALF_WIDTH      0.25f
#define HALF_HEIGHT     0.25f

GameWorld::Kitten::Kitten(GameWorld *aGame) :
game(aGame),
image(game->assets->image("kitten")) {
	auto position = game->assets->userdata("kitten.position")->as<vec2>() - vec(0,0.1f);
	collider = game->collisionSystem.addCollider(aabb(
		position-vec(HALF_WIDTH, 2*HALF_HEIGHT),
		position+vec(HALF_WIDTH, 0)
	), KITTEN_BIT, ENVIRONMENT_BIT, 0, this);
	game->collisionSystem.move(collider, vec(0,0.2f));
}

void GameWorld::Kitten::tick() {

}

void GameWorld::Kitten::draw() {
	auto p = PIXELS_PER_METER * collider->box.bottomCenter();
	game->batch.drawImageScaled(image, p, flipped ? vec(-1,1) : vec(1,1), frame);	
}