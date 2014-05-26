// Little Polygon SDK
// Copyright (C) 2013 Max Kaufmann
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "littlepolygon/context.h"

static Viewport makeView() {
	int w, h; SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &w, &h);
	return Viewport(w, h, 0, 0);
}

//#if LITTLE_POLYGON_MOBILE
//static int handleAppEvents(void *userdata, SDL_Event *event) {
//	switch (event->type) {
//		case SDL_QUIT:
//			exit(0);
//			break;
//			
//		case SDL_APP_TERMINATING:
//			// shut down everything
//			return 0;
//			
//		case SDL_APP_LOWMEMORY:
//			// release as much as possible?
//			return 0;
//			
//		case SDL_APP_WILLENTERBACKGROUND:
//			return 0;
//			
//		case SDL_APP_DIDENTERBACKGROUND:
//			// 5s to save state or you are dead
//			return 0;
//			
//		case SDL_APP_WILLENTERFOREGROUND:
//			return 0;
//			
//		case SDL_APP_DIDENTERFOREGROUND:
//			return 0;
//			
//		default:
//			// just put event on the queue
//			return 1;
//	}
//}
//#endif

// http://devnewton.bci.im/en/gamepad_db
// http://dl.bci.im/gamepad_db/gamepad_db.sdl
const char *gamepadMapping = R"SDLMAP(
341a3608000000000000504944564944,Afterglow PS3 Controller,a:b1,b:b2,x:b0,y:b3,start:b9,guide:b12,back:b8,dpup:h0.1,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b6,righttrigger:b7
0003:046D:C218,Logitech Logitech RumblePad 2 USB Controllers,a:b1,b:b2,x:b0,y:b3,start:b9,back:b8,leftshoulder:b4,rightshoulder:b5,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b6,righttrigger:b7
0003:07B5:0312,Mega World USB Game Controllers,a:b2,b:b3,x:b0,y:b1,start:b9,back:b8,leftshoulder:b4,rightshoulder:b6,leftx:a0,lefty:a1,rightx:a3,righty:a2,lefttrigger:b5,righttrigger:b7
0003:06A3:0107.0008,SAITEK P220,a:b2,b:b3,x:b0,y:b1,start:b4,back:b5,leftshoulder:b6,rightshoulder:b7,leftx:a0,lefty:a1
0003:1A34:0802,USB GAMEPAD 8116,a:b0,b:b1,x:b2,y:b3,guide:b9,back:b8,dpup:a6,dpleft:a5,dpdown:a6,dpright:a5,leftshoulder:b4,rightshoulder:b5,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a4
0003:044F:B315.0006,Mega World Thrustmaster dual analog 3.2,a:b0,b:b2,x:b1,y:b3,start:b9,back:b8,dpup:h0.1,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,leftshoulder:b4,rightshoulder:b6,leftstick:b10,rightstick:b11,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b5,righttrigger:b7
030000004f04000009d0000000010000,Thrustmaster Run'N' Drive Wireless PS3,a:b1,b:b2,x:b0,y:b3,start:b9,guide:b12,back:b8,dpup:h0.1,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b6,righttrigger:b7
030000004f04000008d0000000010000,Thrustmaster Run'N' Drive Wireless,a:b1,b:b2,x:b0,y:b3,start:b9,back:b8,dpup:h0.1,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,leftx:a0,lefty:a1,rightx:a2,righty:a5,lefttrigger:b6,righttrigger:b7
6d0400000000000018c2000000000000,Logitech Rumble Gamepad F510(Mac),a:b1,b:b2,x:b0,y:b3,start:b9,back:b8,dpup:h0.1,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b6,righttrigger:b7
6d0400000000000018c2000000000000,Logitech Rumble Pad 2(Mac),a:b1,b:b2,x:b0,y:b3,start:b9,back:b8,dpup:h0.1,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b6,righttrigger:b7
4c050000000000006802000000000000,PLAYSTATION(R)3 Controller(Mac),a:b14,b:b13,x:b15,y:b12,start:b3,guide:b16,back:b0,dpup:b4,dpleft:b7,dpdown:b6,dpright:b5,leftshoulder:b10,rightshoulder:b11,leftstick:b1,rightstick:b2,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b8,righttrigger:b9
030000006d0400001ec2000020200000,Logitech Rumble Gamepad F510(Linux),a:b0,b:b1,x:b2,y:b3,start:b7,guide:b8,back:b6,dpup:h0.1,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5
030000005e0400008e02000014010000,Microsoft Xbox 360 Gamepad (xpad) (Linux),a:b0,b:b1,x:b2,y:b3,start:b7,guide:b8,back:b6,dpup:h0.1,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5
030000008f0e00001200000010010000,GreenAsia Inc.,a:b2,b:b1,x:b3,y:b0,start:b9,back:b8,dpup:h0.1,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,leftshoulder:b6,rightshoulder:b7,leftstick:b10,rightstick:b11,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b4,righttrigger:b5
)SDLMAP";

SDLContext::SDLContext(const char *caption, int w, int h) {
	
	if (w == 0) {
		// iphone 5 resolution :P
		w = 1136;
		h = 640;
	}
	

	
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GameControllerAddMapping(gamepadMapping);
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024);
	
//	SDL_SetEventFilter(handleAppEvents, 0);
//	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
//	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
//	SDL_Window *pWindow = SDL_CreateWindow(
//		"", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0,
//		SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN|SDL_WINDOW_BORDERLESS|SDL_WINDOW_SHOWN
//	);
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	window = SDL_CreateWindow(
		caption ? caption : "Little Polygon Context",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		w, h, SDL_WINDOW_OPENGL
	);
	
	gl = SDL_GL_CreateContext(window);
	glewExperimental = GL_TRUE;
	glewInit();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	
}

SDLContext::~SDLContext() {
	SDL_GL_DeleteContext(gl);
	SDL_DestroyWindow(window);
	Mix_CloseAudio();
	SDL_Quit();
}

LPContext::LPContext(const char *caption, int w, int h, const char *assetPath, int plotterCap, int linesCap) :
Singleton<LPContext>(this),
SDLContext(caption, w, h),
assets(assetPath),
view(makeView()),
plotter(plotterCap),
lines(linesCap),
sprites(&plotter),
splines(&plotter)
{
}
