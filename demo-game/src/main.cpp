#include <littlepolygon/context.h>

static bool sDone = false;

static void handleKeydown(const SDL_KeyboardEvent& event) {
	switch(event.keysym.sym) {
		case SDLK_ESCAPE:
			sDone = true;
			break;
		
		default:
			break;
	}
}

static void handleEvents() {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_KEYDOWN:
				handleKeydown(event.key);
				break;
			
			case SDL_QUIT:
				sDone = true;
				break;
				
			default:
				break;
		}
	}
}

int main(int argc, char *argv[]) {
	lpInitialize("Hello, World", 900, 550, "assets.bin");
	glClearColor(0.2, 0.2, 0.3, 0);
	
	gView.setSizeWithWidth(300);
	
	while(!sDone) {
		glClear(GL_COLOR_BUFFER_BIT);
		handleEvents();
		
		gSprites.begin(gView);
		int mx, my; SDL_GetMouseState(&mx, &my);
		gSprites.drawImage(gAssets.image("btnUp"), 0.333f * vec(mx, my));
		gSprites.end();
		
		SDL_GL_SwapWindow(SDL_GL_GetCurrentWindow());
	}
	
	lpFinalize();
}


