#include "GameWorld.h"

GameWorld::GameWorld(AssetBundle *aAssets) : 
assets(aAssets), 
env(this),
hero(this),
kitten(this) {

	Mix_Music *music = Mix_LoadMUS("song.mid");
	if(music) { Mix_FadeInMusic(music, -1, 5000); }
}

void GameWorld::tick() {
	timer.tick();
	handleEvents();
	if (timer.deltaTicks > 8) {
		hero.tick();
	}
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
				if (!event.key.repeat) {
					handleKeyDownEvent(event.key);
				}
				break;
		}
	}

}

void GameWorld::handleKeyDownEvent(const SDL_KeyboardEvent& key) {
	switch(key.keysym.sym) {
		case SDLK_ESCAPE:
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
}

void GameWorld::draw() {
	auto scrolling = vec(0,0);
	auto canvasSize = vec(CANVAS_WIDTH, CANVAS_HEIGHT);
	
	batch.begin(canvasSize, scrolling);
	env.draw();
	hero.draw();
	kitten.draw();
	batch.end();

	#if DEBUG
	if (drawWireframe) {
		wireframe.begin(canvasSize, scrolling);
		collisionSystem.debugDraw(wireframe);
		wireframe.end();
	}
	#endif
}




