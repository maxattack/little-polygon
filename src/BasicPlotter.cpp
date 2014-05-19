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

BasicPlotter::BasicPlotter(int aCapacity) : bound(0), capacity(aCapacity), shader(BASIC_SHADER), currentArray(0) {
	vertices = (BasicVertex*) LITTLE_POLYGON_MALLOC(capacity * sizeof(BasicVertex));
	
	// initialize shader
	shader.use();
	uMVP = shader.uniformLocation("mvp");
	uAtlas = shader.uniformLocation("atlas");
	aPosition = shader.attribLocation("aPosition");
	aUV = shader.attribLocation("aUv");
	aColor = shader.attribLocation("aColor");
	
	// initialize buffers
	glGenBuffers(arraysize(arrays), arrays);
	for(int i=0; i<arraysize(arrays); ++i) {
		glBindBuffer(GL_ARRAY_BUFFER, arrays[i]);
		glBufferData(GL_ARRAY_BUFFER, 4*capacity*sizeof(BasicVertex), 0, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

BasicPlotter::~BasicPlotter() {
	glDeleteBuffers(3, arrays);
	LITTLE_POLYGON_FREE(vertices);
}

BasicVertex *BasicPlotter::getVertex(int i) {
	ASSERT(i >= 0);
	ASSERT(i < capacity);
	return vertices + i;
}

void BasicPlotter::begin(const Viewport& aView, GLuint program) {
	ASSERT(!isBound());
	bound = 1;
	view = aView;
	
	if (program) {
		glUseProgram(program);
	} else {
		shader.use();
	}
	// WILL THESE LOCATIONS MATCH??
	glEnableVertexAttribArray(aPosition);
	glEnableVertexAttribArray(aUV);
	glEnableVertexAttribArray(aColor);
	view.setMVP(uMVP);
}

void BasicPlotter::commit(int count) {
	ASSERT(isBound());
	// bind new array buffer, update attribute pointers and buffer data
	glBindBuffer(GL_ARRAY_BUFFER, arrays[currentArray]);
	glVertexAttribPointer(aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(BasicVertex), (GLvoid*)0);
	glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, sizeof(BasicVertex), (GLvoid*)12);
	glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(BasicVertex), (GLvoid*)20);
	glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(BasicVertex), vertices);
	currentArray = (currentArray + 1) % 3;
}

void BasicPlotter::end() {
	ASSERT(isBound());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(aPosition);
	glDisableVertexAttribArray(aUV);
	glDisableVertexAttribArray(aColor);
	bound = 0;
}











