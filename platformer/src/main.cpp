#include "Game.h"
#include <littlepolygon_vmath.h>

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
	
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(2);

	// initialize lp systems
	auto assets = createAssetBundle("platformer.bin");
	auto batch = createSpriteBatch();
	auto plotter = createLinePlotter();
	auto splines = createSplinePlotter();

	// things with ctors
	Timer timer;
	PlayerInput input;
	CollisionSystem* collisions = new CollisionSystem();

	// scene entities
	Environment environment;
	Hero hero;
	Kitten kitten;
	
	environment.init(assets, collisions);
	hero.init(assets, collisions);
	kitten.init(assets, collisions);
	
	// start music
	// Mix_Music *music = Mix_LoadMUS("song.mid");
	// if(music) { Mix_FadeInMusic(music, -1, 5000); }

	while(!input.done) {

		// clear canvas
		glClear(GL_COLOR_BUFFER_BIT);

		// tick time
		timer.tick();
		handleEvents(input);
		hero.tick(&input, collisions, timer.scaledDeltaTime);
		input.jumpPressed = false;

		// render scene
		auto scrolling = vec(0,0);
		auto canvasSize = vec(CANVAS_WIDTH, CANVAS_HEIGHT);

		begin(batch, canvasSize, scrolling);
		environment.draw(batch);
		hero.draw(batch);
		kitten.draw(batch);
		end(batch);

		glDisable(GL_BLEND);

		if (input.drawWireframe) {
			begin(plotter, canvasSize, scrolling);
			collisions->debugDraw(plotter, rgb(0xffff00));
			end(plotter);
		}

		int mx, my;
		SDL_GetMouseState(&mx, &my);
		float px = 0.25f * mx;
		float py = 0.25f * my;

		begin(splines, canvasSize, scrolling);
		drawSpline(
			splines, 
			quadraticBezierMatrix(vec(0,0,0,0), vec(px, py, 0, 0), vec(canvasSize.x, canvasSize.y, 0, 0)), 
			vec(0,-8,-8,16), 
			rgb(0xff00ff)
		);
		end(splines);

		glEnable(GL_BLEND);

		// present and wait for next frame
		SDL_GL_SwapWindow(window);
	}

	delete collisions;
	destroy(plotter);
	destroy(batch);
	destroy(assets);
	
	return 0;

}
