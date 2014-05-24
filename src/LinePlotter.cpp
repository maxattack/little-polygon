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

const GLchar LINE_VERT[] = GLSL(

uniform mat4 mvp;
in vec2 aPosition;
in vec4 aColor;
out vec4 color;


void main() {
	gl_Position = mvp * vec4(aPosition, 0.0, 1.0);
	color = aColor;
}

);

const GLchar LINE_FRAG[] = GLSL(

in vec4 color;
out vec4 outColor;

void main() {
	outColor = color;
}

);

LinePlotter::LinePlotter(int aCapacity) :
count(-1),
capacity(aCapacity),
shader(LINE_VERT, LINE_FRAG),
vertices(0)

{
	shader.use();
	uMVP = shader.uniformLocation("mvp");
	aPosition = shader.attribLocation("aPosition");
	aColor = shader.attribLocation("aColor");
	vertices = (Vertex*) malloc(sizeof(Vertex) * capacity);
	
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * capacity, 0, GL_DYNAMIC_DRAW);
	
	glEnableVertexAttribArray(aPosition);
	glEnableVertexAttribArray(aColor);
	glVertexAttribPointer(
		aPosition, 2, GL_FLOAT, GL_FALSE,
		sizeof(Vertex),
		(void*) 0
	);
	glVertexAttribPointer(
		aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE,
		sizeof(Vertex),
		(void*) 8
	);
	
	glBindVertexArray(0);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


LinePlotter::~LinePlotter() {
	free(vertices);
}

void LinePlotter::begin(const Viewport& viewport) {
	ASSERT(count == -1);
	count = 0;
	shader.use();
	viewport.setMVP(uMVP);
}

void LinePlotter::plot(vec2 p0, vec2 p1, Color c) {
	ASSERT(count >= 0);
	vertices[2*count  ].set(p0, c);
	vertices[2*count+1].set(p1, c);

	++count;
	if (count == capacity) {
		commitBatch();
	}
}

void LinePlotter::plotLittleBox(vec2 p, float r, Color c) {
	plot(p+vec(-r,-r), p+vec(r,-r), c);
	plot(p+vec(r,-r), p+vec(r,r), c);
	plot(p+vec(r,r), p+vec(-r,r), c);
	plot(p+vec(-r,r), p+vec(-r,-r), c);
}

void LinePlotter::plotArrow(vec2 p0, vec2 p1, float r, Color c) {
	plot(p0, p1, c);
	auto delta = r * (p0 - p1).normalized();
	auto r0 = unitVector(0.25 * M_PI);
	plot(p1, p1 + cmul(delta, r0), c);
	plot(p1, p1 + cmul(delta, r0.conjugate()), c);
}

void LinePlotter::end() {
	ASSERT(count >= 0);
	if (count > 0) {
		commitBatch();
	}
	count = -1;
}

void LinePlotter::commitBatch() {
	ASSERT(count > 0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 2*count*sizeof(Vertex), vertices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(vao);
	glDrawArrays(GL_LINES, 0, 2*count);
	glBindVertexArray(0);
	count = 0;
}

