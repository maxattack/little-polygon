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

void Viewport::setFromWindow() {
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	SDL_Point sz;
	SDL_GetWindowSize(win, &sz.x, &sz.y);
	mSize = sz;
	mOffset = vec(0,0);
}

void Viewport::setSizeWithHeight(double h) {
	int ww, wh;
	SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &ww, &wh);
	mSize = vec(h * float(ww) / float(wh), h);
}

void Viewport::setSizeWithWidth(double w) {
	int ww, wh;
	SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &ww, &wh);
	mSize = vec(w, w * float(wh) / float(ww));
}

vec2d Viewport::windowToViewport(vec2d p) const {
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	SDL_Point sz;
	SDL_GetWindowSize(win, &sz.x, &sz.y);
	return mSize * p / vec2(sz) + mOffset;
}

vec2d Viewport::viewportToWindow(vec2d vp) const {
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	int w,h;
	SDL_GetWindowSize(win, &w, &h);
	return vec2d(w,h) / mSize * (vp - mOffset);
}

vec2d Viewport::mouse() const {
	SDL_Point mp;
	SDL_GetMouseState(&mp.x, &mp.y);
	return windowToViewport(mp);
}

void Viewport::setMVP(GLuint mvp) const {
	auto zfar = 128.0;
	auto znear = -128.0;
	auto fan = zfar + znear;
	auto fsn = zfar - znear;
	auto cext = mOffset + mSize;
	auto t = - (cext + mOffset) / mSize;
	GLfloat orth[16] = {
		float(2.0/mSize.x), 0, 0, 0,
		0, float(-2.0/mSize.y), 0, 0,
		0, 0, float(2.0/fsn), 0,
		float(t.x), float(-t.y), float(-fan/fsn), 1.0f
	};
	glUniformMatrix4fv(mvp, 1, 0, orth);
}

