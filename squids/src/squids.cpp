#include "SplineTree.h"
#include <littlepolygon_sprites.h>
#include <littlepolygon_fk.h>
#include <littlepolygon_utils.h>
#include <littlepolygon_collisions.h>

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
	AssetRef assets = loadAssets("squids.bin");
	SplinePlotterRef splines = createSplinePlotter();
	SpritePlotterRef sprites = createSpritePlotter();
	LinePlotterRef lines = createLinePlotter();
	CirclePlotterRef circles = createCirclePlotter();
	FkTreeRef nodes = createFkContext();
	auto tree = new SplineTree();

	FkNodeRef root = nodes.addNode("root");
	FkNodeRef rotor = root.addNode("rotor");
	FkNodeRef rotor2 = root.addNode("rotor2");
	FkNodeRef rotor2a = rotor2.addNode("rotor2a");
	rotor2a.setPosition(80, 0);

	FkNodeRef n0 = rotor.addNode("n0");
	n0.setPosition(300, 0);
	n0.setAttitude(0, 1);

	FkNodeRef n1 = rotor.addNode("n1");
	n1.setPosition(-300, 0);
	n1.setAttitude(0, 1);

	FkNodeRef n2 = rotor2a.addNode("n2");
	n2.setPosition(0, -200);
	n2.setAttitude(-1, 0);

	FkNodeRef n3 = rotor2a.addNode("n3");
	n3.setPosition(0, 200);
	n3.setAttitude(-1, 0);

	auto k0 = tree->addKnot(n0.cachedTransform(), vec(500,0));
	auto k1 = tree->addKnot(n1.cachedTransform(), vec(500,0));
	auto k2 = tree->addKnot(n2.cachedTransform(), vec(500,0));
	auto k3 = tree->addKnot(n3.cachedTransform(), vec(500,0));

	tree->addSegment(k0, k1);
	tree->addSegment(k1, k2);
	tree->addSegment(k2, k3);
	tree->addSegment(k3, k0);

	auto color = rgb(0x4E9689);
	float dim = 0.9f;
	glClearColor(
		dim * color.red(), 
		dim * color.green(), 
		dim * color.blue(), 
		0.0f
	);

	Timer timer;
	timer.reset();
	
	bool done = false;

	root.setPosition(0.5f * vec(w,h));

	while(!done) {
		handleEvents(&done);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		SDL_Point p;
		SDL_GetMouseState(&p.x, &p.y);
		float yAmount = p.y / float(h);

		timer.timeScale = 0.25 * 1.333 * yAmount;
		timer.tick();


		root.setPosition(easeTowards(root.position(), p, 0.1, 0.001 * timer.deltaTicks));
		rotor.setRotation(M_TAU * timer.seconds);
		rotor2.setRotation(-0.2 * M_TAU * timer.seconds);

		n0.apply(affineRotation(0.2 * M_TAU * timer.deltaSeconds));
		n1.apply(affineRotation(0.333 * M_TAU * timer.deltaSeconds));
		n2.apply(affineRotation(-0.1 * M_TAU * timer.deltaSeconds));
		n3.apply(affineRotation(-0.4 * M_TAU * timer.deltaSeconds));
		nodes.cacheWorldTransforms();

		auto canvasSize = vec(w, h);
		auto scrolling = vec(0,0);

		splines.begin(canvasSize, scrolling);
		splines.plot(
			quadraticBezierMatrix(vec4f(0,0,0,0), vec4f(p.x, p.y, 0, 0), vec4f(canvasSize.x, canvasSize.y, 0, 0)), 
			eccentricStroke(32, -30, 32), 
			rgb(0x4E9689)
		);
		tree->draw(splines, rgb(0x87D69B), 1.5f);
		tree->draw(splines, rgb(0xC3FF68));
		splines.end();

		lines.begin(canvasSize, scrolling);
		tree->drawKnots(lines, rgb(0x222255));
		lines.end();

		circles.begin(canvasSize, scrolling);
		circles.plotFilled(p, 12, rgba(0xF4FCE844));
		circles.plotArc(root.local().t, 24, 28, rgba(0x7ED0D644));
		circles.end();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		
		
		sprites.begin(canvasSize, scrolling);
		sprites.drawImage(assets.image("max"), p);
		sprites.drawLabel(assets.font("droid"), vec(4,4), rgba(0xffffff00), "Hello World!");
		sprites.end();

		glDisable(GL_BLEND);

		SDL_GL_SwapWindow(window);
	}

	delete tree;
	sprites.destroy();
	splines.destroy();
	circles.destroy();
}


