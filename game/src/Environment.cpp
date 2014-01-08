#include "Game.h"

void initialize(Environment& env, AssetBundle& assets, CollisionSystem& collisions) {

	// lookup assets
	env.tmap = assets.tilemap("test");
	env.bg = assets.image("background");

	// left/right walls
	int th = int(CANVAS_HEIGHT * METERS_PER_PIXEL);
	collisions.addCollider(aabb(-1,0,0,th));
	collisions.addCollider(aabb(env.tmap->mw, 0, env.tmap->mw+1, th));

	// content colliders
	auto *cdata = assets.userdata("environment.colliders");
	auto colliders = (AABB*) cdata->data();
	auto ncolliders = cdata->size / sizeof(AABB);
	for(int i=0; i<ncolliders; ++i) {
		collisions.addCollider(colliders[i], ENVIRONMENT_BIT);
	}

}

void draw(Environment& env, SpriteBatch& spriteBatch) {

	// temp: hard-coded
	drawImage(spriteBatch, env.bg, vec(4,CANVAS_HEIGHT));
	drawImageScaled(spriteBatch, env.bg, vec(CANVAS_WIDTH-8, CANVAS_HEIGHT-56), vec(-1,1));
	
	// actual tilemap
	drawTilemap(spriteBatch, env.tmap);
}

