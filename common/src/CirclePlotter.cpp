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

#if VERTEX

uniform mediump mat4 mvp;
attribute mediump vec2 aPosition;
attribute mediump vec4 aColor;

void main() {
	gl_Position = mvp * vec4(aPosition, 0, 1);
	color = aColor;
}

#else

void main() {
	gl_FragColor = color;
}

#endif

)GLSL";


struct CirclePlotter {
	size_t resolution;
	
	GLuint prog;
	GLuint vert;
	GLuint frag;
	GLuint uMVP;
	GLuint aPosition;
	GLuint aColor;

	struct Vertex {
		vec2 position;
		Color color;

		inline void set(vec2 p, Color c) { 
			position = p; 
			color = c; 
		}		
	};

	Vertex headVert;
	Vertex *getVert(int i) { return &headVert + i; }
};

CirclePlotter *createCirclePlotter(size_t resolution) {
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
	context->aColor = glGetAttribLocation(context->prog, "aColor");

	return context;
}

void CirclePlotterRef::destroy() {
	glDeleteProgram(context->prog);
	glDeleteShader(context->vert);
	glDeleteShader(context->frag);	
	LITTLE_POLYGON_FREE(context);
}

void CirclePlotterRef::begin(vec2 canvasSize, vec2 canvasOffset) {
	glUseProgram(context->prog);
	setCanvas(context->uMVP, canvasSize, canvasOffset);

	glEnableVertexAttribArray(context->aPosition);
	glEnableVertexAttribArray(context->aColor);

	glVertexAttribPointer(context->aPosition, 2, GL_FLOAT, GL_FALSE, sizeof(CirclePlotter::Vertex), &context->headVert.position);
	glVertexAttribPointer(context->aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CirclePlotter::Vertex), &context->headVert.color);	
}

// right now I'm just doing one draw call per plot.  if this proves to be a performance bottleneck
// we can use degenerate triangles to combine multiple strips together.

void CirclePlotterRef::plotFilled(vec2 p, float r, Color c, float a1, float a2) {
	// plot eet
	vec2 curr = polar(1, a1);
	float da = (a2 - a1) / float(context->resolution-1);
	vec2 rotor = polar(1, da);
	context->getVert(0)->set(p, c);
	for(int i=0; i<context->resolution; ++i) {
		context->getVert(i+1)->set(p + r * curr, c);
		curr = cmul(curr, rotor);
	}

	// draw eet
	glDrawArrays(GL_TRIANGLE_FAN, 0, context->resolution+1);
}

void CirclePlotterRef::plotArc(vec2 p, float r1, float r2, Color c, float a1, float a2) {
	// plot eet
	vec2 curr = polar(1, a1);
	float da = (a2 - a1) / float(context->resolution-1);
	vec2 rotor = polar(1, da);
	for(int i=0; i<context->resolution; ++i) {
		context->getVert(i+i)->set(p + r1 * curr, c);
		context->getVert(i+i+1)->set(p + r2 * curr, c);
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




