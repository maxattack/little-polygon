#pragma once

#include "config.h"
#include "CollisionSystem.h"

#define CANVAS_WIDTH   320
#define CANVAS_HEIGHT  115

#define ENVIRONMENT_BIT 0x00000001
#define HERO_BIT        0x00000002
#define KITTEN_BIT      0x00000004

class Game;

//----------------------------------------------------------------------

class Environment {
public:
	Environment(Game *game);
	void draw();

private:
	Game *game;
	ImageAsset *bg;
	TilemapAsset *tmap;	
};

//----------------------------------------------------------------------

class Hero {
public:
	Hero(Game *game);
	void tick();
	void draw();

private:
	Game *game;

	// physics parameters
	Collider* collider;
	vec2 speed = vec(0,0);
	bool grounded = true;

	// fx parameters
	ImageAsset *image;
	float framef = 0;
	int frame = 0;
	bool flipped = false;
};

//----------------------------------------------------------------------

class Kitten {
public:
	Kitten(Game *game);
	void tick();
	void draw();
	
private:
	Game *game;

	Collider* collider;

	ImageAsset *image;
	int frame = 0;
	bool flipped = false;
};


class Game {
friend class Environment;
friend class Hero;
friend class Kitten;
public:

	Game(AssetBundle *assets);
	void tick();
	void draw();

	inline bool isDone() const { return done; }

private:

	AssetBundle *assets;
	CollisionSystem collisionSystem;
	Timer timer;	
	SpriteBatch batch;


	bool done = false;
	bool jumpPressed = false;

	Environment env;
	Hero hero;
	Kitten kitten;

	#if DEBUG
	LinePlotter wireframe;
	bool drawWireframe = false;
	#endif

	void handleEvents();
	void handleKeyDownEvent(const SDL_KeyboardEvent& key);
};
