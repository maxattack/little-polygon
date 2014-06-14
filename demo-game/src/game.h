#pragma once
#include <littlepolygon/context.h>
#include <littlepolygon/pools.h>

//--------------------------------------------------------------------------------
// CONSTANTS

#define kPixelsPerMeter				(16.0f)
#define kMetersPerPixel				(1.0f/16.0f)
#define kHeroHeight					(0.8f)
#define kHeroWidth 					(0.5f)
#define kHeroMoveSpeed  			(5.0f)
#define kHeroJumpHeight 			(2.5f)
#define kHeroStepsPerMeter			(3.0f)
#define kHeroShootKickback      	(12.0f)
#define kKittenWidth				(0.8f)
#define kKittenHeight				(0.5f)
#define kKittenMoveSpeed        	(2.0f)
#define kKittenPause            	(1.0f)
#define kKittenStepsPerMeter    	(6.6f)
#define kKittenPickupTime       	(0.2f)
#define kKittenShootSpeed       	(25.0f)
#define kKittenCollisionKickback	(3.0f)
#define kKittenCollisionHeight		(1.0f)
#define kSlop						(0.0001f)
#define kDeadZone       			(0.0001f)
#define kGravity        			(72.0f)

//--------------------------------------------------------------------------------
// WORLD ASSET

struct WorldData {

	// INITIAL LOCATIONS
	lpVec heroPosition;
	lpVec kittenPosition;

	// TILE MASK
	int maskWidth, maskHeight;
	uint8_t *maskBytes;
	
};


//--------------------------------------------------------------------------------
// TILE MASK (for collisions)

class TileMask {
private:
	int mWidth, mHeight;
	uint8_t *bytes;
	
public:
	TileMask(const WorldData& data);
	~TileMask();
	
	int width() const { return mWidth; }
	int height() const { return mHeight; }
	bool get(int x, int y) const;

	void mark(int x, int y);
	void clear(int x, int y);

	bool check(lpVec topLeft, lpVec bottomRight) const;
	
	// TODO: Change args to match check() :P
	bool checkLeft(lpVec bottomLeft, lpVec topRight, float *outResult) const;
	bool checkRight(lpVec bottomLeft, lpVec topRight, float *outResult) const;
	bool checkTop(lpVec bottomLeft, lpVec topRight, float *outResult) const;
	bool checkBottom(lpVec bottomLeft, lpVec topRight, float *outResult) const;

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
	lpVec position;
	lpVec speed;
	lpVec anchor;
	lpVec halfSize;
	
public:
	// POSITION AT ANCHOR'S WORLD-COORDINATE
	Entity(lpVec position, lpVec size);
	
	// GETTERS
	float left() const { return position.x - halfSize.x; }
	float right() const { return position.x + halfSize.x; }
	float bottom() const { return position.y + halfSize.y; }
	float top() const { return position.y - halfSize.y; }
	lpVec pixelPosition() const { return kPixelsPerMeter * (position+anchor); }
	
	bool overlaps(const Entity *other);
	
	// MOVE BASED ON SPEED, COLLIDERS
	void move(int* hitX, int* hitY);
	
	void debugDraw();
};

inline float jumpImpulse(float height) { return -sqrtf(2.0f * height * kGravity); }

//--------------------------------------------------------------------------------
// PLAYER INPUT WRAPPER

class PlayerInput {
private:
	int mDirX, mDirY;
	int mPressedJump, mPressedAction;
	SDL_GameController* gamepad;

public:
	PlayerInput();
	~PlayerInput();
	
	// DPAD
	int dirX() const { return mDirX; }
	int dirY() const { return mDirY; }
	lpVec dir() const { return vec(mDirX, mDirY); }
	
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
	bool handleButtonDown(const SDL_ControllerButtonEvent& event);
	bool handleButtonUp(const SDL_ControllerButtonEvent& event);
};

//--------------------------------------------------------------------------------
// CAMERA CONTROLLER

class Camera {
private:
	float quakeTime, flashTime;
	Color restColor;
	lpVec position;
	
public:
	Camera();

	bool isFlashing() const { return flashTime > 0.0f; }
	
	void quake();
	void flash();
	
	void tick();
};

//--------------------------------------------------------------------------------
// HERO ENTITY

class Hero : public Entity {
private:
	ImageAsset *img;
	int dir;
	float animTime, yScale;
	bool grounded;
	Color tint;
	
public:
	Hero(const WorldData& data);
	
	// METHODS
	void tick();
	void draw();
	
	// GETTERS
	bool isGrounded() const { return grounded; }
	bool isStandingStill() const;
	int carryDirection() const { return dir; }
	lpVec carryAnchor() const;
	
private:
	int getFrame() const;
	void performAction();
};

//--------------------------------------------------------------------------------
// KITTEN ENTITY

class Kitten : public Entity {
public:
	enum Status { Pausing, Walking, Carried, Shooting, Falling };
private:
	ImageAsset *img;
	Status status;
	int dir;
	float timeout, animTime;
	float sentryLeft, sentryRight;

	lpVec carryBasePosition;
	float carryProgress;
	
	
public:
	Kitten(const WorldData& data);

	bool canPickUp() const { return status == Pausing || status == Walking || status == Falling; }
	bool isCarried() const { return status == Carried; }
	
	void pickup();
	void shoot();
	
	void tick();
	void draw();

private:
	void startSentry();
	void tickPausing();
	void tickWalking();
	void tickCarried();
	void tickShooting();
	void tickFalling();
};

//--------------------------------------------------------------------------------
// EXPLOSIONS

class Explosion {
private:
	lpVec position;
	float time;
	
public:
	Explosion() {}
	Explosion(lpVec pos, float delay);
	bool tick();
	void draw();
};

//--------------------------------------------------------------------------------
// ROOT WORLD OBJECT

class World : public Singleton<World> {
public:
	PlayerInput input;
	TileMask mask;
	TilemapAsset* tilemap;
	Camera camera;
	Hero hero;
	Kitten kitten;
	
	ImageAsset *explosionImage;
	CompactPool<Explosion> explosions;
	
private:
	bool debugDraw;
	bool done;
	
public:
	World(const WorldData& data);
	
	void spawnExplosion(lpVec position, float delay=0.0f);
	bool destroyTile(int x, int y);

	void run();
	
private:
	void tick();
	void draw();
	void handleEvents();
	void handleKeydown(const SDL_KeyboardEvent& event);
	
	
};

#define gWorld (World::getInstance())
