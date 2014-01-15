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
#include "littlepolygon_base.h"

// A garbage-bin of additional helpful utilities not necessarily required by LPAAT,
// which I've accumulated and rolled over from project to project :P

//--------------------------------------------------------------------------------
// COROUTINE MACROS
//--------------------------------------------------------------------------------

#define COROUTINE_PARAMETER           int _line;
#define COROUTINE_RESET               _line=0;
#define COROUTINE_DISABLE             _line=-1;
#define COROUTINE_ACTIVE              (_line!=-1)
#define COROUTINE_BEGIN               switch(_line){case 0:;
#define COROUTINE_YIELD               {_line=__LINE__; return; case __LINE__:;}
#define COROUTINE_YIELD_RESULT(_x)    {_line=__LINE__;return (_x); case __LINE__:;}
#define COROUTINE_END                 _line=-1;default:;}

//--------------------------------------------------------------------------------
// TIMING & ANIMATION UTILITIES
//--------------------------------------------------------------------------------

struct Timer {
	// sdl ticks since the timer started
	int ticks;
	int deltaTicks;

	// virtually scaled "seconds" since the timer started
	double timeScale;
	double scaledTime;
	double scaledDeltaTime;

	Timer(float aTimeScale=1) : timeScale(aTimeScale) {
		reset();
	}

	void reset() {
		ticks = SDL_GetTicks();
		deltaTicks = 0;
		scaledTime = 0;
		scaledDeltaTime = 0;
	}

	void skipTicks() {
		// for things, like, coming back from pause
		ticks = SDL_GetTicks();
	}

	void tick() {
		deltaTicks = SDL_GetTicks() - ticks;
		scaledDeltaTime = timeScale * deltaSeconds();
		ticks += deltaTicks;
		scaledTime += scaledDeltaTime;
	}

	double seconds() const { return 0.001 * ticks; }
	double deltaSeconds() const { return 0.001 * deltaTicks; }
};

inline int pingPong(int i, int n) {
	i  = i % (n + n - 2);
	return i >= n ? 2 * (n-1) - i : i;
}	

