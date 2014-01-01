#include "GameWorld.h"

int main(int argc, char *argv[]) {

	Context context("A Girl and Her Cat", 4 * CANVAS_WIDTH, 4 * CANVAS_HEIGHT);

	auto color = rgb(0x95b5a2);
	glClearColor(color.red(), color.green(), color.blue(), 0.0f);
	
	AssetBundle assets;
	assets.load("assets.bin");

	GameWorld world(&assets);

	while(!world.isDone()) {
		glClear(GL_COLOR_BUFFER_BIT);
		world.tick();
		world.draw();
		SDL_GL_SwapWindow(context.window());
	}
	
	return 0;
	
}

