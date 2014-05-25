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

SDLContext::SDLContext(const char *caption, int w, int h) {
	
	if (w == 0) {
		// iphone 5 resolution :P
		w = 1136;
		h = 640;
	}
	
	SDL_Init(SDL_INIT_EVERYTHING);
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
