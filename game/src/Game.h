#pragma once

#include "CollisionSystem.h"

#define CANVAS_WIDTH   320
#define CANVAS_HEIGHT  115

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

};

struct Hero {

	// physics parameters
	Collider *collider;
	vec2 speed;
	bool grounded;

	vec2 position() const {
		return collider->box.bottomCenter();
	}

	// rendering parameters
	ImageAsset *image;
	float framef;
	int frame;
	bool flipped;

	SampleAsset *sfxJump;
	SampleAsset *sfxFootfall;

};

struct Kitten {

	// physics parameters
	Collider *collider;

	vec2 position() const {
		return collider->box.bottomCenter();
	}

	// rendering parameters
	ImageAsset *image;
	int frame;
	bool flipped;
};

void initialize(Environment& environment, AssetBundle& assets, CollisionSystem& collisions);
void initialize(Hero& hero, AssetBundle& assets, CollisionSystem& collisions);
void initialize(Kitten& kitten, AssetBundle& assets, CollisionSystem& collisions);

void tick(Hero& hero, PlayerInput& input, CollisionSystem& collisions, float dt);

void draw(Environment& environment, SpriteBatch& spriteBatch);
void draw(Hero& hero, SpriteBatch& spriteBatch);
void draw(Kitten& kitten, SpriteBatch& spriteBatch);


