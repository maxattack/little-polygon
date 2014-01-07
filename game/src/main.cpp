#include "Game.h"

SpriteBatch *gSpriteBatch = 0;

#if DEBUG
LinePlotter *gPlotter = 0;
#endif

int main(int argc, char *argv[]) {

	auto window = initContext("A Girl and Her Cat", 4 * CANVAS_WIDTH, 4 * CANVAS_HEIGHT);

	static SpriteBatch batch;
	gSpriteBatch = &batch;
	initialize(gSpriteBatch);
	
	#if DEBUG
	static LinePlotter plotter;
	gPlotter = &plotter;
	initialize(gPlotter);
	#endif

	AssetBundle assets;
	initialize(&assets, "assets.bin");

	Game world(&assets);

	while(!world.isDone()) {
		glClear(GL_COLOR_BUFFER_BIT);
		world.tick();
		world.draw();
		SDL_GL_SwapWindow(window);
	}
	
	return 0;

}
