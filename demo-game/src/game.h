#pragma once
#include <littlepolygon/context.h>

//--------------------------------------------------------------------------------
// CONSTANTS

#define kPixelsPerMeter			(16.0f)
#define kMetersPerPixel			(1.0f/16.0f)
#define kHeroHeight				(0.8f)
#define kHeroWidth 				(0.6f)
#define kHeroMoveSpeed  		(5.0f)
#define kHeroJumpHeight 		(2.5f)
#define kHeroStepsPerMeter		(3.0f)
#define kKittenWidth			(0.8f)
#define kKittenHeight			(0.6f)
#define kSlop					(0.0001f)
#define kDeadZone       		(0.0001f)
#define kGravity        		(72.0f)

//--------------------------------------------------------------------------------
// TILE MASK (for collisions)

class TileMask {
public:
	struct Data {
		int w, h;
		uint8_t bytes[1]; // actually variable-length, bounded by w and h
		                  // included like this for alignment purposes.
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

	bool check(vec2 topLeft, vec2 bottomRight) const;
	
	// TODO: Change args to match check() :P
	bool checkLeft(vec2 bottomLeft, vec2 topRight, float *outResult) const;
	bool checkRight(vec2 bottomLeft, vec2 topRight, float *outResult) const;
	bool checkTop(vec2 bottomLeft, vec2 topRight, float *outResult) const;
	bool checkBottom(vec2 bottomLeft, vec2 topRight, float *outResult) const;

	void debugDraw();
	
private:
	bool rawGet(int x, int y) const;
	void getIndices(int x, int y, int *byteIdx, int *localIdx) const;
};

//--------------------------------------------------------------------------------
// BASE MOVEABLE ENTITY

class Entity {
private:
	vec2 mPosition;
	vec2 mSpeed;
	vec2 mOffset;
	vec2 mHalfSize;
	
public:

	Entity(vec2 position, vec2 size);
	
	float left() const { return mPosition.x - mHalfSize.x; }
	float right() const { return mPosition.x + mHalfSize.x; }
	float bottom() const { return mPosition.y + mHalfSize.y; }
	float top() const { return mPosition.y - mHalfSize.y; }
	
	vec2 halfSize() const { return mHalfSize; }
	vec2 centerPosition() const { return mPosition; }
	vec2 pixelPosition() const { return kPixelsPerMeter * (mPosition+mOffset); }
	vec2 speed() { return mSpeed; }
	float speedX() const { return mSpeed.x; }
	float speedY() const { return mSpeed.y; }
	vec2 *pspeed() { return &mSpeed; }
	
	void setPosition(vec2 p) { mPosition = p; }
	void setSpeed(vec2 s) { mSpeed = s; }
	void addSpeed(vec2 s) { mSpeed += s; }

	void move(int* hitX, int* hitY);
	
	void debugDraw();
};

//--------------------------------------------------------------------------------
// PLAYER INPUT WRAPPER

class PlayerInput {
private:
	int mDirX, mDirY;
	int mPressedJump;

public:
	PlayerInput();
	
	// DPAD
	int dirX() const { return mDirX; }
	int dirY() const { return mDirY; }
	vec2 dir() const { return vec(mDirX, mDirY); }
	
	// BUTTONS
	bool pressingLeft() const { return mDirX < 0; }
	bool pressingRight() const { return mDirX > 0; }
	bool pressingUp() const { return mDirY < 0; }
	bool pressingDown() const { return mDirY > 0; }
	bool pressedJump() const { return mPressedJump; }
	
	// METHODS
	void enterFrame();
	bool handleEvent(const SDL_Event& event);

private:
	bool handleKeyDown(const SDL_KeyboardEvent& event);
	bool handleKeyUp(const SDL_KeyboardEvent& event);
};

//--------------------------------------------------------------------------------
// HERO ENTITY

class Hero : public Entity {
private:
	ImageAsset *img;
	int dir;
	float animTime, yScale;
	bool grounded;
	
public:
	Hero();
	
	void tick();
	void draw();
	
	bool isGrounded() const { return grounded; }
	bool isStandingStill() const;
	
private:
	int getFrame() const;
};

//--------------------------------------------------------------------------------
// KITTEN ENTITY

class Kitten : public Entity {
private:
	ImageAsset *img;
	
public:
	Kitten();
	
	void tick();
	void draw();
};

//--------------------------------------------------------------------------------
// FULLY COMPOSED WORLD

class World : public Singleton<World> {
public:
	PlayerInput input;
	TileMask mask;
	Hero hero;
	Kitten kitten;
	
private:
	bool debugDraw;
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

#define gWorld (World::getInstance())
