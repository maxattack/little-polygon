#include "littlepolygon/context.h"

static Viewport makeView(SDL_Window *window) {
	int w, h; SDL_GetWindowSize(window, &w, &h);
	return Viewport(w, h, 0, 0);
}

GlobalContext::GlobalContext(const char *caption, int w, int h, const char *assetPath, int plotterCap, int linesCap, int spriteLayers) :
Singleton<GlobalContext>(this),
window(initContext(caption, w, h)),
assets(assetPath),
view(makeView(window)),
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
