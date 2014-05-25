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

#include "littlepolygon/graphics.h"
#include "littlepolygon/utils.h"
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

Vec2 Viewport::windowToViewport(Vec2 p) const {
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	SDL_Point sz;
	SDL_GetWindowSize(win, &sz.x, &sz.y);
	return mSize * p / Vec2(sz) + mOffset;
}

Vec2 Viewport::viewportToWindow(Vec2 vp) const {
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	int w,h;
	SDL_GetWindowSize(win, &w, &h);
	return Vec2(w,h) / mSize * (vp - mOffset);
}

Vec2 Viewport::cursor() const {
	int x,y; SDL_GetMouseState(&x, &y);
	return windowToViewport(vec(x, y));
}

void Viewport::setMVP(GLuint amvp) const {
	mat4f mvp = mat4f::ortho(
		mOffset.x, mOffset.x+mSize.x,
		mOffset.y+mSize.y, mOffset.y,
		128.0, -128.0
	);
	GLfloat buf[16];
	mvp.store(buf);
	glUniformMatrix4fv(amvp, 1, GL_FALSE, buf);
}

