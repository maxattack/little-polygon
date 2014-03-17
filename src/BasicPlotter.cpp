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

const GLchar* BASIC_SHADER = R"GLSL(
varying mediump vec2 uv;
varying lowp vec4 color;

#if VERTEX

uniform highp mat4 mvp;
attribute mediump vec3 aPosition;
attribute mediump vec2 aUv;
attribute mediump vec4 aColor;

void main() {
	gl_Position = mvp * vec4(aPosition, 1.0);
	color = aColor;
	uv = aUv;
}

#else

uniform lowp sampler2D atlas;

void main() {
	lowp vec4 baseColor = texture2D(atlas, uv);
	// premultiplied alpha version
	//gl_FragColor = vec4(mix(baseColor.rgb, baseColor.a * color.rgb, color.a), baseColor.a);
	gl_FragColor = vec4(mix(baseColor.rgb, color.rgb, color.a), baseColor.a);
}

#endif
)GLSL";

struct BasicPlotter {
	int capacity;
	
	const Viewport* view;
	
	GLuint prog;
	GLuint vert;
	GLuint frag;
	GLuint uMVP;
	GLuint uAtlas;
	GLuint aPosition;
	GLuint aUV;
	GLuint aColor;
	
	GLuint arrays[3]; // triple-buffered
	int currentArray;
	
	BasicVertex vertices[1];
	
};

BasicPlotter *createBasicPlotter(size_t capacity) {
	auto result = (BasicPlotter*) LITTLE_POLYGON_MALLOC(
		sizeof(BasicPlotter) + (capacity-1) * sizeof(BasicVertex)
	);
	
	// initialize shader
	result->capacity = capacity;
	result->view = 0;
	CHECK(compileShader(BASIC_SHADER, &result->prog, &result->vert, &result->frag));
	glUseProgram(result->prog);
	result->uMVP = glGetUniformLocation(result->prog, "mvp");
	result->uAtlas = glGetUniformLocation(result->prog, "atlas");
	result->aPosition = glGetAttribLocation(result->prog, "aPosition");
	result->aUV = glGetAttribLocation(result->prog, "aUv");
	result->aColor = glGetAttribLocation(result->prog, "aColor");
	
	// initialize buffers
	glGenBuffers(arraysize(result->arrays), result->arrays);
	for(int i=0; i<arraysize(result->arrays); ++i) {
		glBindBuffer(GL_ARRAY_BUFFER, result->arrays[i]);
		glBufferData(GL_ARRAY_BUFFER, 4*result->capacity*sizeof(BasicVertex), 0, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	result->currentArray = 0;
	
	return result;
}

void BasicPlotterRef::destroy() {
	if (context) {
		glDeleteProgram(context->prog);
		glDeleteShader(context->vert);
		glDeleteShader(context->frag);
		glDeleteBuffers(3, context->arrays);
		LITTLE_POLYGON_FREE(context);
		context = 0;
	}
}

const Viewport* BasicPlotterRef::view() const {
	return context->view;
}

BasicVertex *BasicPlotterRef::getVertex(int i) {
	ASSERT(i >= 0);
	ASSERT(i < context->capacity);
	return context->vertices + i;
}

bool BasicPlotterRef::isBound() const {
	return context->view != 0;
}

int BasicPlotterRef::capacity() const {
	return context->capacity;
}

void BasicPlotterRef::begin(const Viewport& view, GLuint program) {
	ASSERT(!isBound());
	context->view = &view;
	
	ASSERT(context->prog);
	if (program) {
		glUseProgram(program);
	} else {
		glUseProgram(context->prog);
	}
	glEnableVertexAttribArray(context->aPosition);
	glEnableVertexAttribArray(context->aUV);
	glEnableVertexAttribArray(context->aColor);
	view.setMVP(context->uMVP);
}

void BasicPlotterRef::commit(int count) {
	// bind new array buffer, update attribute pointers and buffer data
	glBindBuffer(GL_ARRAY_BUFFER, context->arrays[context->currentArray]);
	glVertexAttribPointer(context->aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(BasicVertex), (GLvoid*)0);
	glVertexAttribPointer(context->aUV, 2, GL_FLOAT, GL_FALSE, sizeof(BasicVertex), (GLvoid*)12);
	glVertexAttribPointer(context->aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(BasicVertex), (GLvoid*)20);
	glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(BasicVertex), context->vertices);
	context->currentArray = (context->currentArray + 1) % 3;
}

void BasicPlotterRef::end() {
	ASSERT(isBound());
	context->view = 0;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(context->aPosition);
	glDisableVertexAttribArray(context->aUV);
	glDisableVertexAttribArray(context->aColor);
}











