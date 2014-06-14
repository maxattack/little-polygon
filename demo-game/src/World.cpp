#include "game.h"

World::World(const WorldData& data) :
Singleton<World>(this),
mask(data),
tilemap(lpAssets.tilemap("test")),
hero(data),
kitten(data),

explosionImage(lpAssets.image("explosion")),
explosions(8),

debugDraw(false),
done(false)
{
}

void World::spawnExplosion(lpVec position, float delay) {
	explosions.alloc(kPixelsPerMeter * position, delay);
}

bool World::destroyTile(int x, int y) {
	if (x >= 0 && x < tilemap->mw && y >= 0 && y < tilemap->mh && tilemap->tileAt(x,y).isDefined()) {
		tilemap->clearTile(x,y);
		mask.clear(x,y);
		auto tileCenter = (vec(x,y)+vec(0.5f, 0.5f));
		camera.flash();
		spawnExplosion(tileCenter);
		spawnExplosion(tileCenter + randomPointInsideCircle(0.2f), randomValue(1.0f, 2.0f));
		spawnExplosion(tileCenter + randomPointInsideCircle(0.4f), randomValue(2.5f, 4.0f));
		lpAssets.sample("explosionSfx")->play();
		return true;
	} else {
		return false;
	}
}

void World::run() {
	
	for(int i=0; i<MIX_CHANNELS; ++i) {
		Mix_Volume(i, 50);
	}
	auto mus = Mix_LoadMUS("song.mid");
	if (mus) {
		Mix_PlayMusic(mus, -1);
	}
	
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
	for(auto e=explosions.begin(); e!=explosions.end();) {
		if (e->tick()) { ++e; } else { explosions.release(e); }
	}
	//gView.setOffset(gView.offset() + vec(4.0f * gTimer.deltaSeconds, 0));
}

void World::draw() {
	
	if (!camera.isFlashing()) {
	
		lpSprites.begin(lpView);
		auto bg = lpAssets.image("background");
		lpSprites.drawImage(bg, vec(0, lpView.height()-16));
		lpSprites.drawTilemap(tilemap);
		kitten.draw();
		hero.draw();
		for(auto& e : explosions) { e.draw(); }
		lpSprites.end();
		
	}

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

