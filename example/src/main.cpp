#include <Box2D/Box2D.h>
#include <littlepolygon_utils.h>

//------------------------------------------------------------------------------
// PHYSICS DIAGNOSTICS
//------------------------------------------------------------------------------

class WireframeDraw : public b2Draw {
public:
	WireframeDraw();

	void Begin(vec2 canvasSize, vec2 canvasOffset=vec(0,0));
	void End();

	void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
	void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
	void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color);
	void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);
	void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
	void DrawTransform(const b2Transform& xf);

private:
	bool readyToDraw;
	LinePlotter plotter;
};

WireframeDraw::WireframeDraw() : readyToDraw(false) {
}

void WireframeDraw::Begin(vec2 canvasSize, vec2 canvasOffset) {
	ASSERT(!readyToDraw);
	readyToDraw = true;
	plotter.begin(canvasSize, canvasOffset);
}

void WireframeDraw::End() {
	ASSERT(readyToDraw);
	readyToDraw = false;
	plotter.end();
}


void WireframeDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
	ASSERT(readyToDraw);
	for(int i=0; i<vertexCount-1; ++i) {
		plotter.plot(vertices[i], vertices[i+1], color);
	}
	plotter.plot(vertices[vertexCount-1], vertices[0], color);
}

void WireframeDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
	ASSERT(readyToDraw);
	//LOG_MSG("FILLED BOX");
}

void WireframeDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) {
	ASSERT(readyToDraw);
	const int resolution = 32;
	auto curr = vec(radius, 0);
	auto rotator = polar(1, 1.0f/resolution);
	for(int i=0; i<resolution; ++i) {
		auto next = cmul(curr, rotator);
		plotter.plot(center + curr, center + next, color);
	}
}

void WireframeDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) {
	ASSERT(readyToDraw);
	//LOG_MSG("SOLIDE CIRCLE");
}

void WireframeDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {
	ASSERT(readyToDraw);
	plotter.plot(p1, p2, color);
}

void WireframeDraw::DrawTransform(const b2Transform& xf) {
	ASSERT(readyToDraw);
	plotter.plot(xf.p, xf.p + 8 * vec(xf.q.c, xf.q.s), rgb(0xff0000));
	plotter.plot(xf.p, xf.p + 8 * vec(-xf.q.s, xf.q.c), rgb(0x00ff00));
}

//------------------------------------------------------------------------------
// ENTRYPOINT
//------------------------------------------------------------------------------

int main(int argc, char *argv[]) {

	Context context("A Girl and Her Cat", 1200, 500);
	AssetBundle assets;
	assets.load("assets.bin");
	auto hero = assets.image("hero");
	auto kitten = assets.image("kitten");
	auto jump = assets.sample("jump");

	SpriteBatch batch;

	// setup physics
	b2World world(vec(0,80));
	WireframeDraw wireframe;
	world.SetDebugDraw(&wireframe);
	wireframe.SetFlags(
		b2Draw::e_shapeBit |
		b2Draw::e_jointBit |
		b2Draw::e_aabbBit  |
		b2Draw::e_pairBit  |
		b2Draw::e_centerOfMassBit
	);



	// create some test bodies
	b2BodyDef groundParams;
	groundParams.position.Set(64, 64);
	auto ground = world.CreateBody(&groundParams);

	b2PolygonShape box;
	box.SetAsBox(16, 16);
	ground->CreateFixture(&box, 1);

	// start streaming music
	Mix_Music *music = Mix_LoadMUS("song.mid");
	if(music) { Mix_PlayMusic(music, -1); }
	
	// start timer
	Timer timer;
	timer.init();

	// initialize canvas
	glClearColor(0.25f, 0.35f, 0.5f, 0.0f);

	bool done = false;
	while(!done) {
		glClear(GL_COLOR_BUFFER_BIT);
		
		timer.tick();
		
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					done = true;
					break;
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_q) {
						done = true;
					} else {
						jump->play();
					}
					break;
				default:
					// do nothing
					break;
			}
		}

		batch.begin(vec(300, 125));
		float hue = fmodf(360 * timer.seconds(), 360);
		batch.drawImage(hero, vec(32, 125), 0, hsva(hue, 1.f, 1.f, 0.5f));

		int mx, my;
		SDL_GetMouseState(&mx, &my);
		batch.drawImage(kitten, 0.25f*vec(mx, my), int(fmod(10*timer.seconds(), kitten->nframes)));

		batch.drawLabelCentered(
			assets.font("default"), 
			vec(150,1), 
			rgba(0xffffffff), 
			"Testing Line Plotting..."
		);

		batch.end();

		wireframe.Begin(vec(300, 125));
		world.DrawDebugData();
		wireframe.End();

		SDL_GL_SwapWindow(context.window());
	}

	return 0;
}

