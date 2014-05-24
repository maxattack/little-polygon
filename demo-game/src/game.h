#pragma once
#include <littlepolygon/context.h>
#include <vector>

#define PixelsPerMeter	(16.0f)
#define MetersPerPixel	(1.0f/16.0f)
#define HeroHeight		(0.8f)
#define HeroWidth 		(0.6f)
#define KittenWidth     (0.8f)
#define KittenHeight    (0.6f)

class TileMask {
public:
	struct Data {
		int w, h;
		uint8_t bytes[1];
	};
	
private:
	int mWidth, mHeight;
	uint8_t *bytes;
	
public:
	TileMask(int w, int h);
	TileMask(const Data* data);
	~TileMask();
	
	int width() const { return mWidth; }
	int height() const { return mHeight; }
	bool get(int x, int y) const;

	void mark(int x, int y);
	void clear(int x, int y);

private:
	void getIndices(int x, int y, int *byteIdx, int *localIdx) const;
};

class Entity {
private:
	vec2 mPosition;
	vec2 mSpeed;
	vec2 mLocalCenter;
	vec2 mHalfSize;
	
public:

	Entity(vec2 position, vec2 size);
		
	vec2 localCenter() const { return mLocalCenter; }
	vec2 halfSize() const { return mHalfSize; }
	vec2 position() const { return mPosition; }
	vec2 speed() const { return mSpeed; }
	
	void setPosition(vec2 p) { mPosition = p; }
	void setSpeed(vec2 s) { mSpeed = s; }
	void addSpeed(vec2 s) { mSpeed += s; }
	void tick(const TileMask *mask);
	
	void debugDraw();
};

class PlayerInput {
private:
	int xdir;
	int pressedJump;

public:
	
};

class Hero : public Entity {
private:
	ImageAsset *img;

public:
	Hero();
	
	void tick();
	void draw();
};

class Kitten : public Entity {
private:
	ImageAsset *img;
	
public:
	Kitten();
	
	void tick();
	void draw();
};

class World : public Singleton<World> {
private:
	TileMask mask;
	Hero hero;
	Kitten kitten;
	bool done;
	
	
public:
	World();
	void run();

private:
	void tick();
	void draw();
	void handleEvents();
	void handleKeydown(const SDL_KeyboardEvent& event);
	
	
};