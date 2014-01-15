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

	FkNodeRef root = createNode(nodes);
	FkNodeRef rotor = createNode(nodes, root);
	FkNodeRef rotor2 = createNode(nodes, root);
	FkNodeRef rotor2a = createNode(nodes, rotor2);
	rotor2a.setPosition(80, 0);

	FkNodeRef n0 = createNode(nodes, rotor);
	n0.setPosition(300, 0);
	n0.setAttitude(10, 500);

	FkNodeRef n1 = createNode(nodes, rotor);
	n1.setPosition(-300, 0);
	n1.setAttitude(10, 500);

	FkNodeRef n2 = createNode(nodes, rotor2a);
	n2.setPosition(0, -200);
	n2.setAttitude(-600, -100);

	FkNodeRef n3 = createNode(nodes, rotor2a);
	n3.setPosition(0, 200);
	n3.setAttitude(-500, 100);

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

		root.setPosition(easeTowards(root.position(), p, 0.1, timer.deltaSeconds()));
		rotor.setRotation(M_TAU * timer.scaledTime);
		rotor2.setRotation(-0.2 * M_TAU * timer.scaledTime);

		n0.apply(affineRotation(0.2 * M_TAU * timer.scaledDeltaTime));
		n1.apply(affineRotation(0.333 * M_TAU * timer.scaledDeltaTime));
		n2.apply(affineRotation(-0.1 * M_TAU * timer.scaledDeltaTime));
		n3.apply(affineRotation(-0.4 * M_TAU * timer.scaledDeltaTime));
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