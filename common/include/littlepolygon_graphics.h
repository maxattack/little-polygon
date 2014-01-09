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
#include "littlepolygon_assets.h"
#include "littlepolygon_utils.h"
#include "littlepolygon_vmath.h"

//------------------------------------------------------------------------------
// UTIL FUNCTIONS.
//------------------------------------------------------------------------------

// initialize everything the asset system needs (SDL, MIXER, etc).  Uses atexit()
// to register teardown.
SDL_Window *initContext(const char *caption, int w=0, int h=0);

// uniform methods for setting orthographic cancas parameters
void setCanvas(GLuint uMVP, vec2 canvasSize, vec2 canvasOffset);

// Dead-simple shader-compiler.  Easiest to just use C++11 raw string
// literals to store the source, or else you could stash it in asset 
// userdata if it's configurable from content.  Uses a VERTEX conditional-
// compilation macro to differentiate the vertex and fragment shader.
bool compileShader(const GLchar* source, GLuint *outProg, GLuint *outVert, GLuint *outFrag);

//------------------------------------------------------------------------------
// SPRITE RENDERING
//------------------------------------------------------------------------------

// This object can render lots of sprites in a small number of batched draw calls
// by coalescing adjacent draws into larger logical draws.
// TODO: perform batch-level clipping?
struct SpriteBatch;
SpriteBatch *createSpriteBatch(int capacity=64);
void destroy(SpriteBatch *context);

// Call this method to initialize the graphics context state.  Coordinates are
// set to a orthogonal projection matrix, and some basic settings like blending are
// enabled.  Any additional state changes can be set *after* this function but *before*
// issuing any draw calls.
void begin(SpriteBatch* context, vec2 canvasSize, vec2 scrolling=vec(0,0));

// Draw the given image.  Will potentially cause a draw call to actually be emitted
// to the graphics device if: (i) the buffer has reached capacity or (ii) the texture 
// atlas has changed.  Color transforms *can* be batched, because they are encoded
// in the vertices, not in shader uniforms.
void drawImage(SpriteBatch* context, ImageAsset *image, vec2 position, int frame=0, Color color=rgba(0x00000000));
void drawImageTransformed(SpriteBatch* context, ImageAsset *image, vec2 position, vec2 attitude, int frame=0, Color color=rgba(0x00000000));
void drawImageRotated(SpriteBatch* context, ImageAsset *image, vec2 position, float radians, int frame=0, Color color=rgba(0x00000000));
void drawImageScaled(SpriteBatch* context, ImageAsset *image, vec2 position, vec2 k, int frame=0, Color color=rgba(0x00000000));
void drawLabel(SpriteBatch* context, FontAsset *font, vec2 p, Color c, const char *msg);
void drawLabelCentered(SpriteBatch* context, FontAsset *font, vec2 p, Color c, const char *msg);
void drawTilemap(SpriteBatch* context, TilemapAsset *map, vec2 position=vec(0,0));

// if you want to monkey with the global rendering state you need to flush
// the render queue first
void flush(SpriteBatch* context);

// Commit the current draw queue and return the graphics context state to it's
// canonical form, to play nice with other renderers.
void end(SpriteBatch* context);

//------------------------------------------------------------------------------
// DEBUG LINE RENDERING
//------------------------------------------------------------------------------

// This is mainly a debugging tool for things like b2DebugDraw and diagnostics,
// so it's not exactly ninja'd for performance.
struct LinePlotter;
LinePlotter *createLinePlotter();
void destroy(LinePlotter *plotter);

void begin(LinePlotter* context, vec2 canvasSize, vec2 canvasOffset=vec(0,0));
void plot(LinePlotter* context, vec2 p0, vec2 p1, Color c);
void end(LinePlotter* context);

//------------------------------------------------------------------------------
// SPLINE REDNERING
//------------------------------------------------------------------------------

// This is a handy way to render splines without uploading vertices.  A single
// static vertex batch is buffered with "parametric coordinates" of the form
// <x^3, x^2, x^1, 1> which are "programmed" by a unifrm hermite matrix.  Supports
// Non-uniform tapering as well for making things like tentacles :P

struct SplinePlotter;
SplinePlotter *createSplinePlotter(int resolution=128);
void destroy(SplinePlotter *context);

void begin(SplinePlotter *context, vec2 canvasSize, vec2 canvasOffset=vec(0,0));
void drawSpline(SplinePlotter *context, mat4 positionMatrix, vec4 strokeVector, Color c);
void end(SplinePlotter *context);


