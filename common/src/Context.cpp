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

#include "littlepolygon_utils.h"

Context *Context::pInst = 0;

#if LITTLE_POLYGON_MOBILE
static int handleAppEvents(void *userdata, SDL_Event *event) {
    switch (event->type) {
		case SDL_QUIT:
			exit(0);
			break;
			
		case SDL_APP_TERMINATING:
			// shut down everything
			return 0;
		case SDL_APP_LOWMEMORY:
			// release as much as possible?
			return 0;
		case SDL_APP_WILLENTERBACKGROUND:
			return 0;
		case SDL_APP_DIDENTERBACKGROUND:
			// 5s to save state or you are dead
			return 0;
		case SDL_APP_WILLENTERFOREGROUND:
			return 0;
		case SDL_APP_DIDENTERFOREGROUND:
			return 0;
		default:
			// just put event on the queue
			return 1;
    }
}
#endif

Context::Context(const char *caption, int w, int h) {
	ASSERT(pInst == 0);
	pInst = this;
	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024);

	#if LITTLE_POLYGON_MOBILE

	SDL_SetEventFilter(handleAppEvents, 0);	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	pWindow = SDL_CreateWindow(
		"", 0, 0, 0, 0, 
		SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN|SDL_WINDOW_BORDERLESS
	);

	#else

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	if (w == 0) {
		// iphone 5 resolution :P
		w = 1136;
		h = 640;
	}
	pWindow = SDL_CreateWindow(
		caption ? caption : "Little Polygon Context", 
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
		w, h, SDL_WINDOW_OPENGL
	);

	#endif

	SDL_GL_CreateContext(pWindow);

	#if !LITTLE_POLYGON_MOBILE
	glewInit();
	#endif
	
	glViewport(0, 0, w, h);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_VERTEX_ARRAY);
}

Context::~Context() {
	Mix_CloseAudio();
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
	pInst = 0;
}



