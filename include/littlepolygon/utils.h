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
#include "base.h"
#include "math.h"

//--------------------------------------------------------------------------------
// HASHING

inline uint32_t fnv1a(const char* name) {
	// inlined so that the compiler can constant-fold over string literals
	uint32_t hval = 0x811c9dc5;
	while(*name) {
		hval ^= (*name);
		hval *= 0x01000193;
		++name;
	}
	return hval;
}


//--------------------------------------------------------------------------------
// COROUTINE MACROS

#define COROUTINE_PARAMETER           int _line;
#define COROUTINE_RESET               _line=0;
#define COROUTINE_INIT                _line(0)
#define COROUTINE_DISABLE             _line=-1;
#define COROUTINE_ACTIVE              (_line!=-1)
#define COROUTINE_BEGIN               switch(_line){case 0:;
#define COROUTINE_YIELD               {_line=__LINE__; return; case __LINE__:;}
#define COROUTINE_YIELD_RESULT(_x)    {_line=__LINE__;return (_x); case __LINE__:;}
#define COROUTINE_END                 _line=-1;default:;}
#define COROUTINE_FINISHED            (_line == -1)

//--------------------------------------------------------------------------------
// TIMING & ANIMATION UTILITIES

struct Timer {
	// sdl ticks since the timer started
	int ticks;
	int deltaTicks;
	
	lpFloat rawSeconds() const { return 0.001f * ticks; }
	lpFloat rawDeltaSeconds() const { return 0.001f * deltaTicks; }
	
	// virtually scaled "seconds" since the timer started
	lpFloat timeScale;
	lpFloat seconds;
	lpFloat deltaSeconds;
	
	Timer(lpFloat aTimeScale=1.0f);
	void reset();
	void tick();

	// for things, like, coming back from pause
	void skipTicks();
};

inline int pingPong(int i, int n) {
	i  = i % (n + n - 2);
	return i >= n ? 2 * (n-1) - i : i;
}

//------------------------------------------------------------------------------
// Singleton Template

template<typename T>
class Singleton {
private:
	static T* inst;
	struct ContextGuard {
		ContextGuard(T* g) { ASSERT(inst == 0); inst = g; }
		~ContextGuard() { inst = 0; }
	};
	ContextGuard contextGuard;


public:
	Singleton(T* aThis) : contextGuard(aThis) {}
	static inline T* getInstancePtr() { return inst; }
	static inline T& getInstance() { ASSERT(inst); return *inst; }
};

template<typename T> T* Singleton<T>::inst = nullptr;






