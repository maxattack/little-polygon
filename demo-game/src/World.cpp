#include "game.h"

World::World() :
Singleton<World>(this),
mask(gAssets.userdata("world.mask")->as<TileMask::Data>()),
debugDraw(false),
done(false)
{
	auto color = gAssets.palette("global")->getColor(0);
	glClearColor(color.red(), color.green(), color.blue(), 0);
	gView.setSizeWithHeight(8 * kPixelsPerMeter);
}

void World::run() {
	
	for(int i=0; i<MIX_CHANNELS; ++i) {
		Mix_Volume(i, 33);
	}
//	auto mus = Mix_LoadMUS("song.mid");
//	if (mus) {
//		Mix_PlayMusic(mus, -1);
//	}
	
	while(!done) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		gTimer.tick();
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
	//gView.setOffset(gView.offset() + vec(4.0f * gTimer.deltaSeconds, 0));
}

void World::draw() {
	
	gSprites.begin(gView);
	auto bg = gAssets.image("background");
	gSprites.drawImage(bg, vec(0, gView.height()-16));
	gSprites.drawTilemap(gAssets.tilemap("test"));
	kitten.draw();
	hero.draw();
	gSprites.end();

	if (debugDraw) {
		Viewport simView(
			gView.size() * kMetersPerPixel,
			gView.offset() * kMetersPerPixel
		);
		gLines.begin(simView);
		kitten.debugDraw();
		hero.debugDraw();
		mask.debugDraw();
		gLines.end();
	}
	
}

void World::handleKeydown(const SDL_KeyboardEvent& event) {
	if (event.repeat) { return; }
	switch(event.keysym.sym) {
		case SDLK_ESCAPE:
			done = true;
			break;
		
		case SDLK_TAB:
			debugDraw = !debugDraw;
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

