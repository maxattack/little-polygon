#include <littlepolygon/context.h>

// #include <mono/jit/jit.h>
// #include <mono/metadata/assembly.h>

#define CANVAS_WIDTH   320
#define CANVAS_HEIGHT  115

#define PIXELS_PER_METER 16
#define METERS_PER_PIXEL (1.0/16.0)

static bool gDone = 0;

void handleEvents();
void handleKeydown(const SDL_KeyboardEvent& event);

void handleEvents() {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			
			case SDL_KEYDOWN:
				handleKeydown(event.key);
				break;
			
			case SDL_QUIT:
				gDone = true;
				break;

			default:
				break;

		}
	}
}

void handleKeydown(const SDL_KeyboardEvent& event) {
	switch(event.keysym.sym) {
		
		case SDLK_ESCAPE:
			gDone = true;
			break;

		default:
			break;
	}
}

// extern "C" void sayHello() {
// 	puts("sayHello() called!");
// }

int main(int argc, char *argv[]) {

	// INTIAILIZE LITTLE POLYGON
	LPInit("Little Polygon Demo", 3 * CANVAS_WIDTH, 3 * CANVAS_HEIGHT, "platformer.bin");

	// INITIALIZE GLOBAL OPENGL STATE
	glClearColor(0.5, 0.6, 0.8, 0.0);

	// auto domain = mono_jit_init("game");
	// auto assembly = mono_domain_assembly_open(domain, "game.dll");
	// if (!assembly) {
	// 	LOG(("NO game.dll\n"));
	// } else {
	// 	const char *monoArgs[] = { "game.dll", "--gc=sgen", "-v" };
	// 	mono_jit_exec(domain, assembly, 3, (char**) monoArgs);
	// }

	// mono_jit_cleanup(domain);

	// INITIALIZE MUSIC
	auto music = Mix_LoadMUS("song.mid");
	if (music) {
		Mix_PlayMusic(music, -1);
	}

	// EVENT LOOP
	while (!gDone) {
		glClear(GL_COLOR_BUFFER_BIT);
		handleEvents();
		SDL_GL_SwapWindow(gWindow);
	}

	// SHUT EVERYTHING DOWN
	LPDestroy();
	
	return 0;

}
