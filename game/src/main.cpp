#include "Game.h"

static AssetBundle assets;
static SpriteBatch batch;
static LinePlotter plotter;
static CollisionSystem collisions;
static Environment environment;
static Hero hero;
static Kitten kitten;

static void handleEvents(PlayerInput& input);
static void handleKeyDown(PlayerInput& input, const SDL_KeyboardEvent& key);

static void handleEvents(PlayerInput& input) {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT:
				input.done = true;
				break;
			case SDL_KEYDOWN:
				if (!event.key.repeat) {
					handleKeyDown(input, event.key);
				}
				break;
		}
	}		
}

static void handleKeyDown(PlayerInput& input, const SDL_KeyboardEvent& key) {
	switch(key.keysym.sym) {
		case SDLK_ESCAPE:
		case SDLK_q:
			input.done = true;
			break;
		case SDLK_TAB:
			input.drawWireframe = !input.drawWireframe;
			break;
		case SDLK_SPACE:
			input.jumpPressed = true;
			break;
	}
}


int main(int argc, char *argv[]) {

	auto window = initContext("A Girl and Her Cat", 4 * CANVAS_WIDTH, 4 * CANVAS_HEIGHT);

	auto color = rgb(0x95b5a2);
	glClearColor(color.red(), color.green(), color.blue(), 0.0f);
	
	// initialize scene
	Timer timer;
	PlayerInput input;
	initialize(assets, "assets.bin");
	initialize(batch);
	initialize(plotter);
	initialize(environment, assets, collisions);
	initialize(hero, assets, collisions);
	initialize(kitten, assets, collisions);
	
	// start music
	Mix_Music *music = Mix_LoadMUS("song.mid");
	if(music) { Mix_FadeInMusic(music, -1, 5000); }

	while(!input.done) {

		// clear canvas
		glClear(GL_COLOR_BUFFER_BIT);

		// tick time
		timer.tick();
		handleEvents(input);
		tick(hero, input, collisions, timer.scaledDeltaTime);
		input.jumpPressed = false;

		// render scene
		auto scrolling = vec(0,0);
		auto canvasSize = vec(CANVAS_WIDTH, CANVAS_HEIGHT);

		begin(batch, canvasSize, scrolling);
		draw(environment, batch);
		draw(hero, batch);
		draw(kitten, batch);
		end(batch);

		if (input.drawWireframe) {
			begin(plotter, canvasSize, scrolling);
			collisions.debugDraw(plotter);
			end(plotter);
		}

		// present and wait for next frame
		SDL_GL_SwapWindow(window);
	}
	
	return 0;

}
