#include "Game.h"

void Environment::init(AssetBundle* assets, CollisionSystem* collisions) {

	// lookup assets
	tmap = assets->tilemap("test");
	bg = assets->image("background");

	// left/right walls
	int th = int(CANVAS_HEIGHT * METERS_PER_PIXEL);
	collisions->addCollider(aabb(-1,0,0,th));
	collisions->addCollider(aabb(tmap->mw, 0, tmap->mw+1, th));

	// content colliders
	auto *cdata = assets->userdata("environment.colliders");
	auto colliders = (AABB*) cdata->data();
	auto ncolliders = cdata->size / sizeof(AABB);
	for(int i=0; i<ncolliders; ++i) {
		collisions->addCollider(colliders[i], ENVIRONMENT_BIT);
	}

}

void Environment::draw(SpritePlotter* spriteBatch) {

	// temp: hard-coded
	drawImage(spriteBatch, bg, vec(4,CANVAS_HEIGHT));
	drawImageScaled(spriteBatch, bg, vec(CANVAS_WIDTH-8, CANVAS_HEIGHT-56), vec(-1,1));
	
	// actual tilemap
	flush(spriteBatch);
	glDisable(GL_BLEND);
	drawTilemap(spriteBatch, tmap);
	flush(spriteBatch);
	glEnable(GL_BLEND);
}

