#include <littlepolygon_fk.h>
#include <littlepolygon_graphics.h>

static void handleKeyDown(const SDL_KeyboardEvent& key, bool *outDone) {
	switch(key.keysym.sym) {
		case SDLK_ESCAPE:
		case SDLK_q:
			*outDone = 1;
			break;
	}
}

static void handleEvents(bool *outDone) {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT:
				*outDone = 1;
				break;
			case SDL_KEYDOWN:
				if (!event.key.repeat) {
					handleKeyDown(event.key, outDone);
				}
				break;
		}
	}		
}

int main(int argc, char *argv[]) {
	auto window = initContext("GameObject Demo", 800, 800);

	// auto goContext = createGoContext(0, 0);
	// createGameObject(goContext, "Squid");
	//auto lines = createLinePlotter();
	auto splines = createSplinePlotter();
	auto circles = createCirclePlotter();
	
	auto color = rgb(0x95b5a2);
	glClearColor(color.red(), color.green(), color.blue(), 0.0f);

	bool done = false;
	while(!done) {
		handleEvents(&done);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		SDL_Point p;
		SDL_GetMouseState(&p.x, &p.y);

		auto canvasSize = vec(800, 800);
		auto scrolling = vec(0,0);

		begin(splines, canvasSize, scrolling);
		drawSpline(
			splines, 
			quadraticBezierMatrix(vec4f(0,0,0,0), vec4f(p.x, p.y, 0, 0), vec4f(canvasSize.x, canvasSize.y, 0, 0)), 
			eccentricStroke(32, -30, 32), 
			rgb(0xff00ff)
		);
		end(splines);

		begin(circles, canvasSize, scrolling);
		plotFilled(circles, p, 12, rgb(0xffffaa));
		plotArc(circles, p, 24, 28, rgb(0xffaaff));
		end(circles);

		SDL_GL_SwapWindow(window);
	}
}