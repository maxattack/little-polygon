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

vec2 Viewport::windowToViewport(vec2 p) const {
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	SDL_Point sz;
	SDL_GetWindowSize(win, &sz.x, &sz.y);
	return mSize * p / vec2(sz) + mOffset;
}

vec2 Viewport::viewportToWindow(vec2 vp) const {
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	int w,h;
	SDL_GetWindowSize(win, &w, &h);
	return vec2(w,h) / mSize * (vp - mOffset);
}

vec2 Viewport::mouse() const {
	SDL_Point mp;
	SDL_GetMouseState(&mp.x, &mp.y);
	return windowToViewport(mp);
}

void Viewport::setMVP(GLuint amvp) const {
	
	mat4f mvp = mat4f::ortho(
		mOffset.x, mOffset.x+mSize.x,
		mOffset.y+mSize.y, mOffset.y,
		128.0, -128.0
	);
	
	
//	if (mRotation < -0.0001 || mRotation > 0.0001) {
//		auto half = (mOffset + 0.5 * mSize);
//		mvp =
//			mvp *
//			mat4f::translation(vec3f(half.x, half.y, 0)) *
//			mat4f::axisRotation(mRotation, vec3f(0,0,-1)) *
//			mat4f::translation(vec3f(-half.x, -half.y, 0));
//	}
	GLfloat buf[16];
	mvp.store(buf);
	glUniformMatrix4fv(amvp, 1, 0, buf);
}

