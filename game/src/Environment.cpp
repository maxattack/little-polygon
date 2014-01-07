#include "Game.h"

Environment::Environment(Game *aGame) : game(aGame) {

	tmap = game->assets->tilemap("test");
	bg = game->assets->image("background");

	// left/right walls
	int th = int(CANVAS_HEIGHT * METERS_PER_PIXEL);
	game->collisionSystem.addCollider(aabb(-1,0,0,th));
	game->collisionSystem.addCollider(aabb(tmap->mw, 0, tmap->mw+1, th));


	// content colliders
	auto *cdata = game->assets->userdata("environment.colliders");
	auto colliders = (AABB*) cdata->data();
	auto ncolliders = cdata->size / sizeof(AABB);
	for(int i=0; i<ncolliders; ++i) {
		game->collisionSystem.addCollider(colliders[i], ENVIRONMENT_BIT);
	}
}

void Environment::draw() {
	game->batch.drawImage( bg, vec(4,CANVAS_HEIGHT) );
	game->batch.drawImageScaled( bg, vec(CANVAS_WIDTH-8, CANVAS_HEIGHT-56), vec(-1,1) );
	game->batch.drawTilemap( tmap );
}

