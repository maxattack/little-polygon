#pragma once

#include "config.h"
#include "collisions.h"

//------------------------------------------------------------------------------
// ASSET USERDATA
//------------------------------------------------------------------------------

struct ColliderData {
	uint32_t x0;
	uint32_t y0;
	uint32_t x1;
	uint32_t y1;

	vec2 position() const { return vec(x0, y0); }
	vec2 size() const { return vec(x1-x0+1, y1-y0+1); }
	
	AABB box() const { return aabb(vec(x0,y0), vec(x1+1, y1+1)); }
};

struct GameWorld;

struct Hero {
	void init(GameWorld *game);
	void tick(GameWorld *game);
	void draw(GameWorld *game);

	// physics parameters
	ID collider;
	vec2 speed;
	bool grounded;

	// fx parameters
	ImageAsset *image;
	float framef;
	int frame;
	bool flipped;
};

struct GameWorld {

	GameWorld(AssetBundle *assets);
	void tick();
	void draw();

	CollisionSystem collisionSystem;
	Timer timer;	
	SpriteBatch batch;

	Hero hero;

	AssetBundle *assets;
	ImageAsset *kitten;
	SampleAsset *jump;
	TilemapAsset *tmap;
	FontAsset *font;

	bool done;
	bool jumpPressed;

#if DEBUG
	bool drawWireframe;
	LinePlotter wireframe;
#endif

	void handleEvents();

};
