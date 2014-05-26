#include "game.h"

PlayerInput::PlayerInput() :
mDirX(0), mDirY(0),
mPressedJump(false), mPressedAction(false),
gamepad(0)
{
	for(int i=0; i<SDL_NumJoysticks(); ++i) {
		if (SDL_IsGameController(i)) {
			gamepad = SDL_GameControllerOpen(i);
			if (gamepad) { break; }
		}
	}
}

PlayerInput::~PlayerInput() {
//	if (gamepad) {
//		SDL_GameControllerClose(gamepad);
//	}
}

void PlayerInput::enterFrame() {
	mPressedJump = false;
	mPressedAction = false;
	
}

bool PlayerInput::handleEvent(const SDL_Event& event) {
	switch(event.type) {
		case SDL_KEYDOWN: return handleKeyDown(event.key);
		case SDL_KEYUP: return handleKeyUp((event.key));
		case SDL_CONTROLLERBUTTONDOWN: return handleButtonDown(event.cbutton);
		case SDL_CONTROLLERBUTTONUP: return handleButtonUp(event.cbutton);
		default: return false;
	}
}

bool PlayerInput::handleKeyDown(const SDL_KeyboardEvent& event) {
	if (event.repeat) { return true; }
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
			mPressedAction = true;
			mDirY = 1;
			return true;
		
		case SDLK_UP:
		case SDLK_w:
			mPressedJump = true;
			mDirY = -1;
			return true;
		
		case SDLK_SPACE:
			mPressedJump = true;
			return true;
		
		case SDLK_z:
			mPressedAction = true;
			return true;
			
		default:
			return false;
			
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
		
			
		default:
			return false;
			
	}
}

bool PlayerInput::handleButtonDown(const SDL_ControllerButtonEvent& event) {
	switch(event.button) {
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			mDirX = -1;
			return true;
			
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			mDirX = 1;
			return true;
		
		case SDL_CONTROLLER_BUTTON_A:
			mPressedJump = true;
			return true;
		
		case SDL_CONTROLLER_BUTTON_X:
			mPressedAction = true;
			return true;
			
		default: return false;
	}
}

bool PlayerInput::handleButtonUp(const SDL_ControllerButtonEvent& event) {
	switch(event.button) {
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			if (mDirX == -1) { mDirX = 0; }
			return true;
			
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			if (mDirX == 1) { mDirX = 0; }
			return true;

		default: return false;
	}
}


