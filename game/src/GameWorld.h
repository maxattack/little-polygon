#pragma once

#include "config.h"
#include "CollisionSystem.h"

#define CANVAS_WIDTH   320
#define CANVAS_HEIGHT  115

#define ENVIRONMENT_BIT 0x00000001
#define HERO_BIT        0x00000002
#define KITTEN_BIT      0x00000004

class GameWorld {
public:

	//------------------------------------------------------------------
	// INNER TYPES

	class Environment {
	public:
		Environment(GameWorld *game);
		void draw();

	private:
		GameWorld *game;
		ImageAsset *bg;
		TilemapAsset *tmap;	
	};

	//------------------------------------------------------------------

	class Hero {
	public:
		Hero(GameWorld *game);
		void tick();
		void draw();

	private:
		GameWorld *game;

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

	//------------------------------------------------------------------

	class Kitten {
	public:
		Kitten(GameWorld *game);
		void tick();
		void draw();
		
	private:
		GameWorld *game;

		Collider* collider;

		ImageAsset *image;
		int frame = 0;
		bool flipped = false;
	};

	//------------------------------------------------------------------
	// PUBLIC INTERFACE

	GameWorld(AssetBundle *assets);
	void tick();
	void draw();

	inline bool isDone() const { return done; }

private:

	//------------------------------------------------------------------
	// PARAMETERS

	AssetBundle *assets;
	CollisionSystem collisionSystem;
	Timer timer;	
	SpriteBatch batch;

public:

	// order-of-initialization matters here

	Environment env;
	Hero hero;
	Kitten kitten;

private:

	bool done = false;
	bool jumpPressed = false;

	#if DEBUG
	LinePlotter wireframe;
	bool drawWireframe = false;
	#endif

	//------------------------------------------------------------------
	// HELPER METHODS

	void handleEvents();
	void handleKeyDownEvent(const SDL_KeyboardEvent& key);
};
