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
#include "littlepolygon_math.h"
#include "littlepolygon_graphics.h"
#include <algorithm>

Viewport::Viewport() : mOffset(0,0) {
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	SDL_Point sz;
	SDL_GetWindowSize(win, &sz.x, &sz.y);
	mSize = sz;
}

void Viewport::setSizeWithHeight(float h) {
	int ww, wh;
	SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &ww, &wh);
	mSize = vec(h * float(ww) / float(wh), h);
}

void Viewport::setSizeWithWidth(float w) {
	int ww, wh;
	SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &ww, &wh);
	mSize = vec(w, w * float(wh) / float(ww));
}

vec2 Viewport::windowToViewport(vec2 p) const {
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	SDL_Point sz;
	SDL_GetWindowSize(win, &sz.x, &sz.y);
	return mSize * p / vec2(sz) + mOffset;
}

vec2 Viewport::viewportToWindow(vec2 vp) const {
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	SDL_Point sz;
	SDL_GetWindowSize(win, &sz.x, &sz.y);
	return vec2(sz) / mSize * (vp - mOffset);
}

vec2 Viewport::mouse() const {
	SDL_Point mp;
	SDL_GetMouseState(&mp.x, &mp.y);
	return windowToViewport(mp);
}

void Viewport::setMVP(GLuint mvp) const {
	float zfar = 128;
	float znear = -128;
	float fan = zfar + znear;
	float fsn = zfar - znear;
	vec2 cext = mOffset + mSize;
	vec2 t = - (cext + mOffset) / mSize;
	GLfloat orth[16] = {
		2.f/mSize.x, 0, 0, 0,
		0, -2.f/mSize.y, 0, 0,
		0, 0, 2.f/fsn, 0,
		t.x, -t.y, -fan/fsn, 1
	};
	glUniformMatrix4fv(mvp, 1, 0, orth);
}

