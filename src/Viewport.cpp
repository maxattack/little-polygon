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

void Viewport::setFromWindow()
{
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	SDL_Point sz;
	SDL_GetWindowSize(win, &sz.x, &sz.y);
	mSize.x = (lpFloat)sz.x;
	mSize.y = (lpFloat)sz.y;
	mOffset = vec(0,0);
}

void Viewport::setSizeWithHeight(lpFloat h)
{
	int ww, wh;
	SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &ww, &wh);
	mSize = vec(h * lpFloat(ww) / lpFloat(wh), h);
}

void Viewport::setSizeWithWidth(lpFloat w)
{
	int ww, wh;
	SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &ww, &wh);
	mSize = vec(w, w * lpFloat(wh) / lpFloat(ww));
}

Vec2 Viewport::windowToViewport(Vec2 p) const
{
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	int szx, szy;  SDL_GetWindowSize(win, &szx, &szy);
	return mSize * p / Vec2((lpFloat) szx, (lpFloat)szy) + mOffset;
}

Vec2 Viewport::viewportToWindow(Vec2 vp) const
{
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	int w,h;
	SDL_GetWindowSize(win, &w, &h);
	return vec((lpFloat)w,(lpFloat)h) / mSize * (vp - mOffset);
}

Vec2 Viewport::cursor() const
{
	int x,y; SDL_GetMouseState(&x, &y);
	return windowToViewport(vec((lpFloat)x, (lpFloat)y));
}

void Viewport::setMVP(GLuint amvp) const
{
	lpFloat zfar = 128;
	lpFloat znear = -128;
	lpFloat fan = zfar + znear;
	lpFloat fsn = zfar - znear;
	Vec2 cext = mOffset + mSize;
	Vec2 t = - (cext + mOffset) / mSize;
	GLfloat buf[16] = {
		2.f/mSize.x, 0, 0, 0,
		0, -2.f/mSize.y, 0, 0,
		0, 0, 2.f/fsn, 0,
		t.x, -t.y, -fan/fsn, 1
	};
	glUniformMatrix4fv(amvp, 1, GL_FALSE, buf);
}

