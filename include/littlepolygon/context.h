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

#pragma once

// THIS IS STRICTLY A CONVENIENCE MODULE :P

#include "assets.h"
#include "events.h"
#include "graphics.h"
#include "sprites.h"
#include "utils.h"


class GlobalContext : public Singleton<GlobalContext> {
private:
	struct SDLContext {
		SDL_Window *window;
		SDL_GLContext gl;
		SDLContext(const char *caption, int w, int h);
		~SDLContext();
	};

public:
	SDLContext sdl;
	AssetBundle assets;
	Viewport view;
	Timer timer;
	TimerQueue queue;
//	BasicPlotter plotter;
	LinePlotter lines;
	SpritePlotter sprites;
	SpriteBatch batch;
//	SplinePlotter splines;
	

public:
	GlobalContext(const char *caption, int w, int h, const char *assetPath, int plotterCap, int linesCap, int spriteLayers);
	~GlobalContext();
};

inline void lpInitialize(const char *caption, int w, int h, const char *assetPath=0, int plotterCap=128, int linesCap=128, int spriteLayers=8) {
	new GlobalContext(caption, w, h, assetPath, plotterCap, linesCap, spriteLayers);
}

inline void lpFinalize() {
	delete GlobalContext::getInstancePtr();
}

#define gWindow  (GlobalContext::getInstance().sdl.window)
#define gAssets  (GlobalContext::getInstance().assets)
#define gView    (GlobalContext::getInstance().view)
#define gTimer   (GlobalContext::getInstance().timer)
#define gQueue   (GlobalContext::getInstance().queue)
#define gLines   (GlobalContext::getInstance().lines)
#define gSprites (GlobalContext::getInstance().sprites)
#define gBatch   (GlobalContext::getInstance().batch)

