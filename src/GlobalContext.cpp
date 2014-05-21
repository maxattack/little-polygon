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

GlobalContext::SDLContext::SDLContext(const char *caption, int w, int h) {
	
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
	auto pWindow = SDL_CreateWindow(
		caption ? caption : "Little Polygon Context",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		w, h, SDL_WINDOW_OPENGL
	);
	
	SDL_GL_CreateContext(pWindow);

	#if LITTLE_POLYGON_OPENGL_CORE
	glewExperimental = GL_TRUE;
	glewInit();
	#endif
	
	SDL_GetWindowSize(pWindow, &w, &h);
	glViewport(0, 0, w, h);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}



GlobalContext::GlobalContext(const char *caption, int w, int h, const char *assetPath, int plotterCap, int linesCap, int spriteLayers) :
Singleton<GlobalContext>(this),
sdl(caption, w, h),
assets(assetPath),
view(makeView()),
plotter(plotterCap),
lines(linesCap),
sprites(&plotter),
batch(spriteLayers),
splines(&plotter)
{
}

GlobalContext::~GlobalContext() {
  Mix_CloseAudio();
  SDL_Quit();	
}
