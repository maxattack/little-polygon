#include "platformer.h"

void Environment::init(AssetRef assets, CollisionSystemRef collisions) {

	// lookup assets
	tmap = assets.tilemap("test");
	bg = assets.image("background");

	// left/right walls
	int th = int(CANVAS_HEIGHT * METERS_PER_PIXEL);
	collisions.addCollider(aabb(-1,0,0,th));
	collisions.addCollider(aabb(tmap->mw, 0, tmap->mw+1, th));

	// content colliders
	auto cdata = assets.userdata("environment.colliders");
	auto *colliders = cdata->as<AABB>();
	auto ncolliders = cdata->size / sizeof(AABB);
	for(int i=0; i<ncolliders; ++i) {
		collisions.addCollider(colliders[i], ENVIRONMENT_BIT);
	}

}

void Environment::draw(SpritePlotterRef plotter) {

	// temp: hard-coded
	plotter.drawImage(bg, vec(4,CANVAS_HEIGHT));
	plotter.drawImage(bg, affineTranslation(CANVAS_WIDTH-8, CANVAS_HEIGHT-56) * affineScale(-1,1));
	
	// actual tilemap
	plotter.flush();
	glDisable(GL_BLEND);
	plotter.drawTilemap(tmap);
	plotter.flush();
	glEnable(GL_BLEND);
}

