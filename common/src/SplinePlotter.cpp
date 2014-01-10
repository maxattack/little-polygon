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


// Gift-idea: Specify a texture "ramp"?
const GLchar* SPLINE_SHADER = R"GLSL(
#if VERTEX

attribute mediump vec4 parameter;
attribute mediump float side;

uniform mediump mat4 mvp;
uniform mediump mat4 positionMatrix;
uniform mediump mat4 strokeMatrix;
uniform mediump vec4 strokeVector;

void main() {
	gl_Position = mvp * vec4((
			(positionMatrix * parameter) + 
			(dot(strokeVector, parameter) * side) * normalize(strokeMatrix * parameter)
	).xyz, 1);
}

#else

uniform lowp vec4 color;

void main() {
	gl_FragColor = color;
}

#endif
)GLSL";

struct SplinePlotter {
	int resolution;
	vec2 canvasSize;
	vec2 canvasScroll;

	GLuint prog;
	GLuint vert;
	GLuint frag;

	GLuint aParameter;
	GLuint aSide;

	GLuint uMVP;
	GLuint uPositionMatrix;
	GLuint uStrokeMatrix;
	GLuint uStrokeVector;
	GLuint uColor;

	GLuint arrayBuf;

	struct Vertex {
		float x, y, z, w, side;
	};

};

// helper methods
static SplinePlotter *allocSpline() {
	return (SplinePlotter*) LITTLE_POLYGON_MALLOC(
		sizeof(SplinePlotter)
	);
}

static void dealloc(SplinePlotter *context) {
	LITTLE_POLYGON_FREE(context);
}

static void initialize(SplinePlotter *context, int resolution) {
	context->resolution = resolution;

	// compile the shader
	CHECK( 
		compileShader(SPLINE_SHADER, &context->prog, &context->vert, &context->frag) 
	);
	glUseProgram(context->prog);
	context->uMVP = glGetUniformLocation(context->prog, "mvp");
	context->uPositionMatrix = glGetUniformLocation(context->prog, "positionMatrix");
	context->uStrokeMatrix = glGetUniformLocation(context->prog, "strokeMatrix");
	context->uStrokeVector = glGetUniformLocation(context->prog, "strokeVector");
	context->uColor = glGetUniformLocation(context->prog, "color");
	
	context->aParameter = glGetAttribLocation(context->prog, "parameter");
	context->aSide = glGetAttribLocation(context->prog, "side");

	// compute the static vertex buffer
	glGenBuffers(1, &context->arrayBuf);
	glBindBuffer(GL_ARRAY_BUFFER, context->arrayBuf);
	auto buffer = (SplinePlotter::Vertex*) LITTLE_POLYGON_MALLOC( 
		2 * resolution * sizeof(SplinePlotter::Vertex) 
	);
	for(int i=0; i<resolution; ++i) {
		float u = float(i) / (resolution-1.0f);
		buffer[i+i  ].x = u*u*u;
		buffer[i+i  ].y = u*u;
		buffer[i+i  ].z = u;
		buffer[i+i  ].w = 1;
		buffer[i+i  ].side = -1;
		buffer[i+i+1].x = u*u*u;
		buffer[i+i+1].y = u*u;
		buffer[i+i+1].z = u;
		buffer[i+i+1].w = 1;
		buffer[i+i+1].side = 1;		
	}
	glBufferData(
		GL_ARRAY_BUFFER, 
		2 * resolution * sizeof(SplinePlotter::Vertex), 
		buffer, 
		GL_STATIC_DRAW
	);
	LITTLE_POLYGON_FREE(buffer);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void release(SplinePlotter *context) {
	glDeleteBuffers(1, &context->arrayBuf); 
	glDeleteProgram(context->prog);
	glDeleteShader(context->vert);
	glDeleteShader(context->frag);
}

SplinePlotter *createSplinePlotter(int resolution) {
	auto context = allocSpline();
	initialize(context, resolution);
	return context;
}

void destroy(SplinePlotter *context) {
	release(context);
	dealloc(context);
}

void begin(SplinePlotter *context, vec2 canvasSize, vec2 canvasOffset) {
	ASSERT(context->prog);
	glUseProgram(context->prog);

	context->canvasSize = canvasSize;
	context->canvasScroll = canvasOffset;
	setCanvas(context->uMVP, context->canvasSize, context->canvasScroll);

	glBindBuffer(GL_ARRAY_BUFFER, context->arrayBuf);
	glEnableVertexAttribArray(context->aParameter);
	glEnableVertexAttribArray(context->aSide);
	glVertexAttribPointer(context->aParameter, 4, GL_FLOAT, GL_FALSE, sizeof(SplinePlotter::Vertex), (GLvoid*)0);
	glVertexAttribPointer(context->aSide, 1, GL_FLOAT, GL_FALSE, sizeof(SplinePlotter::Vertex), (GLvoid*)16);
}

void drawSpline(SplinePlotter *context, mat4 positionMatrix, vec4 strokeVector, Color c) {
	glUniformMatrix4fv(context->uPositionMatrix, 1, 0, positionMatrix.m);
	glUniformMatrix4fv(context->uStrokeMatrix, 1, 0, perpendicularMatrix(positionMatrix).m);
	glUniform4fv(context->uStrokeVector, 1, strokeVector.values);
	glUniform4f(context->uColor, c.red(), c.green(), c.blue(), c.alpha());
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 2*context->resolution);
}

void end(SplinePlotter *context) {
	glDisableVertexAttribArray(context->aParameter);
	glDisableVertexAttribArray(context->aSide);
	glUseProgram(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
