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
#include "utils.h"

class SDLContext {
public:
	SDL_Window *window;
	SDL_GLContext gl;
	
	SDLContext(const char *caption, int w, int h);
	~SDLContext();
};

class LPContext : public Singleton<LPContext>, public SDLContext {
public:
	AssetBundle assets;
	Viewport view;
	Timer timer;
	TimerQueue queue;
	Plotter plotter;
	LinePlotter lines;
	SpritePlotter sprites;
	
	LPContext(const char *caption, int w, int h, const char *assetPath, int plotterCap, int linesCap);
};

inline void lpInitialize(const char *caption, int w, int h, const char *assetPath=0, int plotterCap=1024, int linesCap=128) {
	static LPContext ctxt(caption, w, h, assetPath, plotterCap, linesCap);
}

#define lpWindow  (LPContext::getInstance().window)
#define lpAssets  (LPContext::getInstance().assets)
#define lpView    (LPContext::getInstance().view)
#define lpTimer   (LPContext::getInstance().timer)
#define lpQueue   (LPContext::getInstance().queue)
#define lpLines   (LPContext::getInstance().lines)
#define lpSprites (LPContext::getInstance().sprites)
#define lpBatch   (LPContext::getInstance().batch)
