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

// standard includes
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <cstring>
#include <stdint.h>
#include <limits.h>
#if EMSCRIPTEN
	#include <emscripten/emscripten.h>
	#include <SDL/SDL.h>
	#include <SDL2/SDL_opengles2.h>
	#include <SDL/SDL_mixer.h>
#else
	#include <SDL2/SDL.h>
	#if __IPHONEOS__
		#include <SDL2/SDL_opengles2.h>
	#else
		#define GLEW_STATIC
		#include <GL/glew.h>
		#include <SDL2/SDL_opengl.h>
	#endif
	#include <SDL2_mixer/SDL_mixer.h>
#endif
#include <vectorial/vectorial.h>
using namespace vectorial;

// highter level conditional-compilation flags
#if __IPHONEOS__
	#define LITTLE_POLYGON_MOBILE 1
#else
	#define LITTLE_POLYGON_MOBILE 0
#endif
#if LITTLE_POLYGON_MOBILE || EMSCRIPTEN
	#define LITTLE_POLYGON_OPENGL_ES 1
	#define LITTLE_POLYGON_OPENGL_CORE 0
#else
	#define LITTLE_POLYGON_OPENGL_ES 0
	#define LITTLE_POLYGON_OPENGL_CORE 1
#endif

// handy macros
#ifndef STATIC_ASSERT
#define STATIC_ASSERT(_x)  ((void)sizeof(char[1 - 2*!(_x)]))
#endif

#ifndef arraysize
#define arraysize(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef offsetof
#define offsetof(t,m)  ((uintptr_t)(uint8_t*)&(((t*)0)->m))
#endif

#ifndef MIN
#define MIN(a,b)   ((a) < (b) ? (a) : (b))
#define MAX(a,b)   ((a) > (b) ? (a) : (b))
#endif

#ifdef DEBUG
#   define ASSERT(cond)     (assert(cond))
#   define LOG(_x)          printf _x
#   define LOG_MSG(_msg)    printf("%s:%d " _msg "\n", __FILE__, __LINE__)
#	define LOG_INT(_expr)	printf("%s:%d " #_expr " = %d\n", __FILE__, __LINE__, (_expr))
#	define LOG_FLOAT(_expr)	printf("%s:%d " #_expr " = %f\n", __FILE__, __LINE__, (_expr))
#	define LOG_VEC(_expr)	{ vec2 __u__ = (_expr); printf("%s:%d " #_expr " = <%f,%f>\n", __FILE__, __LINE__, __u__.x, __u__.y); }
#else
#   define ASSERT(cond)
#   define LOG(_x)
#   define LOG_MSG(_msg)
#	define LOG_INT(_expr)
#	define LOG_FLOAT(_expr)
#	define LOG_VEC(_expr)
#endif

