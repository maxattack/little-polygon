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
#include "littlepolygon_math.h"

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
	double seconds;
	double rawDeltaSeconds;
	double deltaSeconds;
	double smoothing;
	
	Timer(double aTimeScale=1) :
	ticks(SDL_GetTicks()), deltaTicks(0),
	timeScale(aTimeScale), seconds(0),
	rawDeltaSeconds(0), smoothing(0.1) {
		SDL_DisplayMode dm;
		SDL_GetWindowDisplayMode(SDL_GL_GetCurrentWindow(), &dm);
		if (dm.refresh_rate) {
			deltaSeconds = timeScale * 0.001 * dm.refresh_rate;
		} else {
			deltaSeconds = timeScale/60.0;
		}
	}
	
	void reset() {
		ticks = SDL_GetTicks();
		seconds = 0;
	}

	void skipTicks() {
		// for things, like, coming back from pause
		ticks = SDL_GetTicks();
	}

	void tick() {
		deltaTicks = SDL_GetTicks() - ticks;
		ticks += deltaTicks;
		rawDeltaSeconds = timeScale * (0.001 * deltaTicks);
		seconds += rawDeltaSeconds;
		deltaSeconds = lerpd(deltaSeconds , rawDeltaSeconds, smoothing);
		smoothing *= 0.99;
		smoothing = MAX(smoothing, 0.000001);
	}
};

inline int pingPong(int i, int n) {
	i  = i % (n + n - 2);
	return i >= n ? 2 * (n-1) - i : i;
}

struct Timeout {
	double current, target;
	
	Timeout() : current(0), target(0) {}
	Timeout(double aTarget) : current(0), target(aTarget) {}
	
	double progress() const { return current / target; }
	
	void reset() { current = 0; }
	void reset(double newTarget) { current = 0; target = newTarget; }
	
	bool tick(Timer& timer) {
		current += timer.deltaSeconds;
		if (current >= target) {
			current = target;
			return true;
		} else {
			return false;
		}
	}
};




