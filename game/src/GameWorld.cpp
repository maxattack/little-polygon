#include "game.h"

GameWorld::GameWorld(AssetBundle *aAssets) : 
assets(aAssets), 
done(false) {

#if DEBUG
	drawWireframe = true;
#endif

	// init assets
	hero.init(this);
	kitten = assets->image("kitten");
	jump = assets->sample("jump");
	tmap = assets->tilemap("test");
	font = assets->font("default");

	// init physics
	auto *cdata = assets->userdata("colliders");
	auto colliders = (ColliderData*) cdata->data();
	auto ncolliders = cdata->size / sizeof(ColliderData);
	for(int i=0; i<ncolliders; ++i) {
		auto c = colliders[i];
		collisionSystem.addCollider(c.box());
	}

	// start streaming music
	Mix_Music *music = Mix_LoadMUS("song.mid");
	if(music) { Mix_FadeInMusic(music, -1, 5000); }
	
	timer.reset();
}

void GameWorld::tick() {
	timer.tick();
	handleEvents();
	hero.tick(this);
	jumpPressed = false;	
}

void GameWorld::handleEvents() {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT:
				done = true;
				break;
			case SDL_KEYDOWN:
				if (event.key.repeat) {
					break;
				}
				switch(event.key.keysym.sym) {
					case SDLK_q:
						done = true;
						break;
					case SDLK_TAB:
						drawWireframe = !drawWireframe;
						break;
					case SDLK_SPACE:
						jumpPressed = true;
						break;
				}
				break;
		}
	}

}

void GameWorld::draw() {
	auto scrolling = vec(0,0); //vec(15 * timer.seconds(), 0);
	auto canvasSize = vec(320, 128);
	batch.begin(canvasSize, scrolling);
	batch.drawTilemap( tmap );
	hero.draw(this);
	batch.end();

#if DEBUG
	if (drawWireframe) {
		wireframe.begin(canvasSize, scrolling);
		collisionSystem.debugDraw(wireframe);
		wireframe.end();
	}
#endif

}
