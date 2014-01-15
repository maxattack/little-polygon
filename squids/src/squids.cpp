#include "SplineTree.h"

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
	int w=800, h=600;
	auto window = initContext("GameObject Demo", w, h);
	auto splines = createSplinePlotter();
	auto circles = createCirclePlotter();
	auto nodes = createFkContext();
	auto tree = new SplineTree(nodes);

	auto root = createNode(nodes);
	auto rotor = createNode(nodes, root);
	auto rotor2 = createNode(nodes, root);
	auto rotor2a = createNode(nodes, rotor2);
	setPosition(rotor2a, vec(80, 0));

	auto n0 = createNode(nodes, rotor);
	setPosition(n0, vec(300, 0));
	setAttitude(n0, vec(10, 500));

	auto n1 = createNode(nodes, rotor);
	setPosition(n1, vec(-300, 0));
	setAttitude(n1, vec(10, 500));

	auto n2 = createNode(nodes, rotor2a);
	setPosition(n2, vec(0, -200));
	setAttitude(n2, vec(-600, -100));

	auto n3 = createNode(nodes, rotor2a);
	setPosition(n3, vec(0, 200));
	setAttitude(n3, vec(-500, 100));

	tree->addSegment(n0, n1);
	tree->addSegment(n1, n2);
	tree->addSegment(n2, n3);
	tree->addSegment(n3, n0);

	
	auto color = rgb(0xff77ff);
	glClearColor(color.red(), color.green(), color.blue(), 0.0f);

	Timer timer;
	Mix_Music *music = Mix_LoadMUS("song.mid");
	if(music) { Mix_FadeInMusic(music, -1, 5000); }
	

	bool done = false;
	while(!done) {
		handleEvents(&done);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		timer.tick();

		SDL_Point p;
		SDL_GetMouseState(&p.x, &p.y);

		setPosition(root, p);
		setRotation(rotor, M_TAU * timer.scaledTime);
		setRotation(rotor2, -0.2f * M_TAU * timer.scaledTime);
		cacheWorldTransforms(nodes);

		auto canvasSize = vec(w, h);
		auto scrolling = vec(0,0);

		begin(splines, canvasSize, scrolling);
		drawSpline(
			splines, 
			quadraticBezierMatrix(vec4f(0,0,0,0), vec4f(p.x, p.y, 0, 0), vec4f(canvasSize.x, canvasSize.y, 0, 0)), 
			eccentricStroke(32, -30, 32), 
			rgb(0xff00ff)
		);
		tree->draw(splines, rgb(0xff00ff));
		end(splines);

		begin(circles, canvasSize, scrolling);
		plotFilled(circles, p, 12, rgb(0xffffaa));
		plotArc(circles, p, 24, 28, rgb(0xffaaff));
		end(circles);

		SDL_GL_SwapWindow(window);
	}

	delete tree;
	destroy(splines);
	destroy(circles);
}