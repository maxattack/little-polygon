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
	int w=1000, h=700;
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

	tree->addSegment(fkCachedTransform(n0), fkCachedTransform(n1));
	tree->addSegment(fkCachedTransform(n1), fkCachedTransform(n2));
	tree->addSegment(fkCachedTransform(n2), fkCachedTransform(n3));
	tree->addSegment(fkCachedTransform(n3), fkCachedTransform(n0));

	
	auto color = rgb(0x4E9689);
	float dim = 0.9f;
	glClearColor(
		dim * color.red(), 
		dim * color.green(), 
		dim * color.blue(), 
		0.0f
	);

	Timer timer;
	Mix_Music *music = Mix_LoadMUS("song.mid");
	if(music) { Mix_FadeInMusic(music, -1, 5000); }
	
	bool done = false;

	setPosition(root, 0.5f * vec(w,h));

	while(!done) {
		handleEvents(&done);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		SDL_Point p;
		SDL_GetMouseState(&p.x, &p.y);
		float yAmount = p.y / float(h);

		timer.timeScale = 1.333 * yAmount;
		timer.tick();

		setPosition(root, easeTowards(fkLocal(root).t, p, 0.1f, timer.deltaSeconds()));

		setRotation(rotor, M_TAU * timer.scaledTime);
		setRotation(rotor2, -0.2f * M_TAU * timer.scaledTime);

		setLocal(n0, fkLocal(n0) * affineRotation(0.2 * M_TAU * timer.scaledDeltaTime));
		setLocal(n1, fkLocal(n1) * affineRotation(0.333 * M_TAU * timer.scaledDeltaTime));
		setLocal(n2, fkLocal(n2) * affineRotation(-0.1 * M_TAU * timer.scaledDeltaTime));
		setLocal(n3, fkLocal(n3) * affineRotation(-0.4 * M_TAU * timer.scaledDeltaTime));
		cacheWorldTransforms(nodes);

		auto canvasSize = vec(w, h);
		auto scrolling = vec(0,0);

		begin(splines, canvasSize, scrolling);
		drawSpline(
			splines, 
			quadraticBezierMatrix(vec4f(0,0,0,0), vec4f(p.x, p.y, 0, 0), vec4f(canvasSize.x, canvasSize.y, 0, 0)), 
			eccentricStroke(32, -30, 32), 
			rgb(0x4E9689)
		);
		tree->draw(splines, rgb(0x87D69B), 1.5f);
		tree->draw(splines, rgb(0xC3FF68));
		end(splines);

		glEnable(GL_BLEND);
		begin(circles, canvasSize, scrolling);
		plotFilled(circles, p, 12, rgba(0xF4FCE844));
		plotArc(circles, fkLocal(root).t, 24, 28, rgba(0x7ED0D644));
		end(circles);
		glDisable(GL_BLEND);

		SDL_GL_SwapWindow(window);
	}

	delete tree;
	destroy(splines);
	destroy(circles);
}