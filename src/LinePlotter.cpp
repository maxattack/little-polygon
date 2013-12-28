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

const GLchar* SIMPLE_SHADER = R"GLSL(

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

LinePlotter::LinePlotter() : count(0) {
	CHECK( ShaderAsset::compile(SIMPLE_SHADER, &prog, &vert, &frag) );
	glUseProgram(prog);
	uMVP = glGetUniformLocation(prog, "mvp");
	aPosition = glGetAttribLocation(prog, "aPosition");
	aColor = glGetAttribLocation(prog, "aColor");
}

LinePlotter::~LinePlotter() {
	glDeleteProgram(prog);
	glDeleteShader(vert);
	glDeleteShader(frag);
}

void LinePlotter::begin(vec2 canvasSize, vec2 canvasOffset) {
	glDisable(GL_BLEND);

	glUseProgram(prog);

	GenericShader::setCanvas(uMVP, canvasSize, canvasOffset);

	glEnableVertexAttribArray(aPosition);
	glEnableVertexAttribArray(aColor);

	glVertexAttribPointer(aPosition, 2, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), &vertices[0].position);
	glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SimpleVertex), &vertices[0].color);	
}

void LinePlotter::plot(vec2 p0, vec2 p1, Color c) {
	vertices[2*count  ].set(p0, c);
	vertices[2*count+1].set(p1, c);

	++count;
	if (count == LINE_PLOTTER_CAPACITY) {
		commitBatch();
	}
}

void LinePlotter::end() {
	if (count > 0) {
		commitBatch();
	}
	glDisableVertexAttribArray(aPosition);
	glDisableVertexAttribArray(aColor);
	glUseProgram(0);
}

void LinePlotter::commitBatch() {
	ASSERT(count > 0);
	glDrawArrays(GL_LINES, 0, 2*count);
	count = 0;
}

