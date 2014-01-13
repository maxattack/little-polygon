#include "Game.h"

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

static vec2 mousePosition() {
	int mx, my;
	SDL_GetMouseState(&mx, &my);
	return 0.25f * vec(mx, my);
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
	auto spritePlotter = createSpritePlotter();
	auto linePlotter = createLinePlotter();
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
	hero.init(assets, batch, collisions);
	kitten.init(assets, batch, collisions);
	
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

		begin(spritePlotter, canvasSize, scrolling);
		environment.draw(spritePlotter);
		draw(batch, spritePlotter);
		end(spritePlotter);

		glDisable(GL_BLEND);

		if (input.drawWireframe) {
			begin(linePlotter, canvasSize, scrolling);
			collisions->debugDraw(linePlotter, rgb(0xffff00));
			end(linePlotter);
		}


		auto p = mousePosition();

		begin(splines, canvasSize, scrolling);
		drawSpline(
			splines, 
			quadraticBezierMatrix(vec4f(0,0,0,0), vec4f(p.x, p.y, 0, 0), vec4f(canvasSize.x, canvasSize.y, 0, 0)), 
			eccentricStroke(16, -15.75f, 16), 
			rgb(0xff00ff)
		);
		end(splines);

		glEnable(GL_BLEND);

		// present and wait for next frame
		SDL_GL_SwapWindow(window);
	}

	delete collisions;
	destroy(linePlotter);
	destroy(spritePlotter);
	destroy(assets);
	
	return 0;

}
