#include "game.h"

int main(int argc, char *argv[]) {
	Context context("A Girl and Her Cat", 1280, 512);
	glClearColor(0.25f, 0.35f, 0.5f, 0.0f);
	
	AssetBundle assets;
	assets.load("assets.bin");

	GameWorld world(&assets);

	while(!world.done) {
		glClear(GL_COLOR_BUFFER_BIT);
		world.tick();
		world.draw();
		SDL_GL_SwapWindow(context.window());
	}
	
	return 0;
}

