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

#include "littlepolygon_graphics.h"

const GLchar* CIRCLE_SHADER = R"GLSL(

varying mediump vec4 color;
varying mediump vec2 uv;

#if VERTEX

uniform highp mat4 mvp;
attribute mediump vec2 aPosition;
attribute mediump vec2 aUV;
attribute mediump vec4 aColor;

void main() {
	gl_Position = mvp * vec4(aPosition, 0, 1);
	color = aColor;
	uv = aUV;
}

#else

uniform lowp sampler2D texture;

void main() {
	gl_FragColor = color * texture2D(texture, uv);
}

#endif

)GLSL";


struct CirclePlotter {
	size_t resolution;
	const Viewport *view;
	
	GLuint prog;
	GLuint vert;
	GLuint frag;
	GLuint uMVP;
	GLuint aPosition;
	GLuint aUV;
	GLuint aColor;

	GLuint fakeAntiAlias;
	float fakeAntiAliasFactor;

	struct Vertex {
		vec2 position;
		vec2 uv;
		Color color;

		inline void set(vec2 p, vec2 u, Color c) { 
			position = p; 
			uv = u;
			color = c; 
		}		
	};

	Vertex headVert;
	Vertex *getVert(int i) { return &headVert + i; }
};

CirclePlotterRef createCirclePlotter(size_t resolution) {
	auto context = (CirclePlotter*) LITTLE_POLYGON_MALLOC(
		sizeof(CirclePlotter) + 
		sizeof(CirclePlotter::Vertex) * (2 * resolution-1));
	context->resolution = resolution;

	CHECK( 
		compileShader(CIRCLE_SHADER, &context->prog, &context->vert, &context->frag) 
	);

	glUseProgram(context->prog);
	context->uMVP = glGetUniformLocation(context->prog, "mvp");
	context->aPosition = glGetAttribLocation(context->prog, "aPosition");
	context->aUV = glGetAttribLocation(context->prog, "aUV");
	context->aColor = glGetAttribLocation(context->prog, "aColor");
	
	return context;
}

void CirclePlotterRef::destroy() {
	glDeleteProgram(context->prog);
	glDeleteShader(context->vert);
	glDeleteShader(context->frag);	
	LITTLE_POLYGON_FREE(context);
}


#define FAKE_ANTIALIAS_FACTOR (0.01f)

void CirclePlotterRef::begin(const Viewport& viewport) {
	
	context->view = &viewport;
	
	int w,h;
	SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &w, &h);
	context->fakeAntiAliasFactor = FAKE_ANTIALIAS_FACTOR * float(w) / viewport.width();
	
	glUseProgram(context->prog);
	viewport.setMVP(context->uMVP);

	glEnableVertexAttribArray(context->aPosition);
	glEnableVertexAttribArray(context->aUV);
	glEnableVertexAttribArray(context->aColor);

	glVertexAttribPointer(context->aPosition, 2, GL_FLOAT, GL_FALSE, sizeof(CirclePlotter::Vertex), &context->headVert.position);
	glVertexAttribPointer(context->aUV, 2, GL_FLOAT, GL_FALSE, sizeof(CirclePlotter::Vertex), &context->headVert.uv);
	glVertexAttribPointer(context->aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CirclePlotter::Vertex), &context->headVert.color);
	
	glBindTexture(GL_TEXTURE_2D, getFakeAntialiasTexture());
}

// right now I'm just doing one draw call per plot.  if this proves to be a performance bottleneck
// we can use degenerate triangles to combine multiple strips together.

void CirclePlotterRef::plotFilled(vec2 p, float r, Color c, float a1, float a2) {
	// plot eet
	vec2 curr = unitVector(a1);
	float da = (a2 - a1) / float(context->resolution-1);
	vec2 rotor = unitVector(da);
	float v = clamp(context->fakeAntiAliasFactor * r);
	context->getVert(0)->set(p, vec(0.5, v), c);
	for(int i=0; i<context->resolution; ++i) {
		context->getVert(i+1)->set(p + r * curr, vec(1, v), c);
		curr = cmul(curr, rotor);
	}

	// draw eet
	glDrawArrays(GL_TRIANGLE_FAN, 0, context->resolution+1);
}

void CirclePlotterRef::plotArc(vec2 p, float r1, float r2, Color c, float a1, float a2) {
	// plot eet
	vec2 curr = unitVector(a1);
	float da = (a2 - a1) / float(context->resolution-1);
	vec2 rotor = unitVector(da);
	float v = clamp(context->fakeAntiAliasFactor * 0.5 * fabsf(r2-r1));
	for(int i=0; i<context->resolution; ++i) {
		context->getVert(i+i)->set(p + r1 * curr, vec(0, v), c);
		context->getVert(i+i+1)->set(p + r2 * curr, vec(1, v), c);
		curr = cmul(curr, rotor);
	}

	// draw eet
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 2*context->resolution);

}

void CirclePlotterRef::end() {
	glDisableVertexAttribArray(context->aPosition);
	glDisableVertexAttribArray(context->aColor);
	glUseProgram(0);

}




