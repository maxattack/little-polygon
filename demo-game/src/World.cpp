#include "game.h"

World::World() :
Singleton<World>(this),
mask(gAssets.userdata("world.mask")->as<TileMask::Data>()),
done(false)
{
	glClearColor(0.8, 0.8, 0.9, 0);
	gView.setSizeWithWidth(20 * kPixelsPerMeter);
}

void World::run() {
	while(!done) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		input.enterFrame();
		handleEvents();
		tick();
		draw();
		SDL_GL_SwapWindow(gWindow);
	}
}

void World::tick() {
	hero.tick();
	kitten.tick();
}

void World::draw() {
	
	gSprites.begin(gView);
	gSprites.drawTilemap(gAssets.tilemap("test"));
	kitten.draw();
	hero.draw();
	gSprites.end();

	Viewport simView(
		gView.size() * kMetersPerPixel,
		gView.offset() * kMetersPerPixel
	);
	gLines.begin(simView);
	kitten.debugDraw();
	hero.debugDraw();
	gLines.end();
}

void World::handleKeydown(const SDL_KeyboardEvent& event) {
	switch(event.keysym.sym) {
		case SDLK_ESCAPE:
			done = true;
			break;
			
		default:
			break;
	}
}

void World::handleEvents() {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		if (input.handleEvent(event)) { continue; }
		switch(event.type) {
			case SDL_KEYDOWN:
				handleKeydown(event.key);
				break;
				
			case SDL_QUIT:
				done = true;
				break;
				
			default:
				break;
		}
	}
}

