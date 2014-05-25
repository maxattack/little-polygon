#include "game.h"

World::World(const WorldData& data) :
Singleton<World>(this),
mask(data),
tilemap(lpAssets.tilemap("test")),
hero(data),
kitten(data),
debugDraw(false),
done(false)
{
	auto color = lpAssets.palette("global")->getColor(0);
	glClearColor(color.red(), color.green(), color.blue(), 0);
	lpView.setSizeWithHeight(8 * kPixelsPerMeter);
}


void World::destroyTile(int x, int y) {
	if (x >= 0 && x < tilemap->mw && y >= 0 && y < tilemap->mh) {
		//lpAssets.sample("explosionSfx")->play();
		tilemap->clearTile(x,y);
		mask.clear(x,y);
	}
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
		lpTimer.tick();
		input.enterFrame();
		handleEvents();
		tick();
		draw();
		SDL_GL_SwapWindow(lpWindow);
	}
}

void World::tick() {
	hero.tick();
	kitten.tick();
	camera.tick();
	//gView.setOffset(gView.offset() + vec(4.0f * gTimer.deltaSeconds, 0));
}

void World::draw() {
	
	lpSprites.begin(lpView);
	auto bg = lpAssets.image("background");
	lpSprites.drawImage(bg, vec(0, lpView.height()-16));
	lpSprites.drawTilemap(tilemap);
	kitten.draw();
	hero.draw();
	lpSprites.end();

	if (debugDraw) {
		Viewport simView(
			lpView.size() * kMetersPerPixel,
			lpView.offset() * kMetersPerPixel
		);
		lpLines.begin(simView);
		kitten.debugDraw();
		hero.debugDraw();
		mask.debugDraw();
		lpLines.end();
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

