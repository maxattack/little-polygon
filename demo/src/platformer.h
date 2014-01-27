#pragma once

#include <littlepolygon_collisions.h>
#include <littlepolygon_sprites.h>
#include <littlepolygon_utils.h>

#define CANVAS_WIDTH   320
#define CANVAS_HEIGHT  115

#define PIXELS_PER_METER 16
#define METERS_PER_PIXEL (1.0/16.0)

#define ENVIRONMENT_BIT 0x00000001
#define HERO_BIT        0x00000002
#define KITTEN_BIT      0x00000004

struct PlayerInput {
	bool done = false;
	bool jumpPressed = false;
	bool drawWireframe = false;

	int xdir() {
		auto kb = SDL_GetKeyboardState(0);
		return kb[SDL_SCANCODE_LEFT] ? -1 : 
		       kb[SDL_SCANCODE_RIGHT] ? 1 : 0;
	}
};

struct Environment {

	// rendering parameters
	ImageAsset *bg;
	TilemapAsset *tmap;

	void init(AssetRef assets, CollisionSystemRef collisions);
	void draw(SpritePlotterRef plotter);

};

struct Hero {

	// physics parameters
	ColliderRef collider;
	vec2 speed;
	bool grounded;

	// rendering parameters
	AffineMatrix xform;
	SpriteRef sprite;
	float framef;

	SampleAsset *sfxJump;
	SampleAsset *sfxFootfall;

	void init(AssetRef assets, SpriteBatchRef batch, CollisionSystemRef collisions);
	void tick(PlayerInput* input, float dt);
};

struct Kitten {

	// physics parameters
	ColliderRef collider;

	// rendering parameters
	AffineMatrix xform;
	SpriteRef sprite;

	void init(AssetRef assets, SpriteBatchRef batch, CollisionSystemRef collisions);

	void mew() { LOG_MSG("mew!"); }
};
