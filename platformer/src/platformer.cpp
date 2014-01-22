#include "platformer.h"

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
	auto assets = loadAssets("platformer.bin");
	auto batch = createSpriteBatch();
	auto sprites = createSpritePlotter();
	auto lines = createLinePlotter();

	// things with ctors
	Timer timer;
	PlayerInput input;
	auto collisions = createCollisionSystem();
	collisions.setMetersToDisplay(PIXELS_PER_METER);

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
		hero.tick(&input, timer.scaledDeltaTime);
		input.jumpPressed = false;

		// render scene
		auto scrolling = vec(0,0);
		auto canvasSize = vec(CANVAS_WIDTH, CANVAS_HEIGHT);

		sprites.begin(canvasSize, scrolling);
		environment.draw(sprites);
		batch.draw(sprites);
		sprites.end();

		if (input.drawWireframe) {


			glDisable(GL_BLEND);
			lines.begin(canvasSize, scrolling);
			collisions.debugDraw(lines, rgb(0xffff00));

			// test raycasting
			SDL_Point mouse;
			SDL_GetMouseState(&mouse.x, &mouse.y);
			auto ray = Ray(0.25f * canvasSize, 0.25f * vec2(mouse));
			auto physRay = Ray(METERS_PER_PIXEL * ray.p0, METERS_PER_PIXEL * ray.p1);
			ColliderRef result;
			auto u = collisions.raycast(physRay, 0xffffffff, &result);
			if (u > 0) {
				lines.plot(ray.p0, ray.pointAt(u), rgb(0x00ff00));
			} else {
				lines.plot(ray.p0, ray.p1, rgb(0xff0000));
			}

			lines.end();
			glEnable(GL_BLEND);

		}
	
		// present and wait for next frame
		SDL_GL_SwapWindow(window);
	}

	collisions.destroy();
	lines.destroy();
	sprites.destroy();
	batch.destroy();
	assets.destroy();
	
	return 0;

}
