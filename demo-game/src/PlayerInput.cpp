#include "game.h"

PlayerInput::PlayerInput() :
mDirX(0), mDirY(0),
mPressedJump(false)
{
	
}

void PlayerInput::enterFrame() {
	mPressedJump = false;
}

bool PlayerInput::handleEvent(const SDL_Event& event) {
	switch(event.type) {
		case SDL_KEYDOWN: return handleKeyDown(event.key);
		case SDL_KEYUP: return handleKeyUp((event.key));
		default: return false;
	}
}

bool PlayerInput::handleKeyDown(const SDL_KeyboardEvent& event) {
	switch(event.keysym.sym) {
		
		case SDLK_LEFT:
		case SDLK_a:
			mDirX = -1;
			return true;
		
		case SDLK_RIGHT:
		case SDLK_d:
			mDirX = 1;
			return true;
		
		case SDLK_DOWN:
		case SDLK_s:
			mDirY = 1;
			return true;
		
		case SDLK_UP:
		case SDLK_w:
			mDirY = -1;
			return true;
		
		case SDLK_SPACE:
			mPressedJump = true;
			return true;
		
		default: return false;
	}

}

bool PlayerInput::handleKeyUp(const SDL_KeyboardEvent& event) {
	switch(event.keysym.sym) {
		
		case SDLK_LEFT:
		case SDLK_a:
			if (mDirX == -1) { mDirX = 0; }
			return true;
		
		case SDLK_RIGHT:
		case SDLK_d:
			if (mDirX == 1) { mDirX = 0; }
			return true;
		
		case SDLK_DOWN:
		case SDLK_s:
			if (mDirY == 1) { mDirY = 0; }
			return true;
		
		case SDLK_UP:
		case SDLK_w:
			if (mDirY == -1) { mDirY = 0; }
			return true;
		
			
		default: return false;
	}
}

