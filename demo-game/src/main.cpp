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
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		handleEvents();
		
		gSprites.begin(gView);
		gSprites.drawImage(gAssets.image("background"), gView.mouse());
		gSprites.end();
		
		gLines.begin(gView);
		gLines.plot(vec(0,0), gView.mouse(), rgb(0xffff00));
		gLines.end();
		
		SDL_GL_SwapWindow(gWindow);
	}
	
	lpFinalize();
}


