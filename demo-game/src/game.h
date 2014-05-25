#pragma once
#include <littlepolygon/context.h>

//--------------------------------------------------------------------------------
// CONSTANTS

#define kPixelsPerMeter			(16.0f)
#define kMetersPerPixel			(1.0f/16.0f)
#define kHeroHeight				(0.8f)
#define kHeroWidth 				(0.5f)
#define kHeroMoveSpeed  		(5.0f)
#define kHeroJumpHeight 		(2.5f)
#define kHeroStepsPerMeter		(3.0f)
#define kKittenWidth			(0.8f)
#define kKittenHeight			(0.5f)
#define kKittenMoveSpeed        (2.0f)
#define kKittenPause            (1.0f)
#define kKittenStepsPerMeter    (6.6f)
#define kKittenPickupTime       (0.2f)
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
	
	bool isFloor(int x, int y) const;
	
private:
	bool rawGet(int x, int y) const;
	void getIndices(int x, int y, int *byteIdx, int *localIdx) const;
};

//--------------------------------------------------------------------------------
// BASE MOVEABLE ENTITY

class Entity {
public:
	vec2 position;
	vec2 speed;
	vec2 anchor;
	vec2 halfSize;
	
public:
	// POSITION AT ANCHOR'S WORLD-COORDINATE
	Entity(vec2 position, vec2 size);
	
	// GETTERS
	float left() const { return position.x - halfSize.x; }
	float right() const { return position.x + halfSize.x; }
	float bottom() const { return position.y + halfSize.y; }
	float top() const { return position.y - halfSize.y; }
	vec2 pixelPosition() const { return kPixelsPerMeter * (position+anchor); }
	
	bool overlaps(const Entity *other);
	
	// MOVE BASED ON SPEED, COLLIDERS
	void move(int* hitX, int* hitY);
	
	void debugDraw();
};

//--------------------------------------------------------------------------------
// PLAYER INPUT WRAPPER

class PlayerInput {
private:
	int mDirX, mDirY;
	int mPressedJump, mPressedAction;

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
	bool pressedAction() const { return mPressedAction; }
	
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
	bool grounded, holdingKitten;
	
public:
	Hero();
	
	// METHODS
	void tick();
	void draw();
	
	// GETTERS
	bool isGrounded() const { return grounded; }
	bool isStandingStill() const;
	int carryDirection() const { return dir; }
	vec2 carryAnchor() const;
	
private:
	int getFrame() const;
	void performAction();
};

//--------------------------------------------------------------------------------
// KITTEN ENTITY

class Kitten : public Entity {
public:
	enum Status { Pausing, Walking, Carried };
private:
	ImageAsset *img;
	Status status;
	int dir;
	float timeout, animTime;
	float sentryLeft, sentryRight;

	vec2 carryBasePosition;
	float carryProgress;
	
	
public:
	Kitten();

	bool isCarried() const { return status == Carried; }
	
	void pickup();
	
	void tick();
	void draw();

private:
	void tickPausing();
	void tickWalking();
	void tickCarried();
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
