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

GenericShader::GenericShader(const GLchar *source) : prog(0) {
	CHECK(ShaderAsset::compile(source, &prog, &vert, &frag));
	glUseProgram(prog);
	uMVP = glGetUniformLocation(prog, "mvp");
	uAtlas = glGetUniformLocation(prog, "atlas");
	aPosition = glGetAttribLocation(prog, "aPosition");
	aUV = glGetAttribLocation(prog, "aUv");
	aColor = glGetAttribLocation(prog, "aColor");
}

GenericShader::~GenericShader() {
	if (prog) {
		glDeleteProgram(prog);
		glDeleteShader(vert);
		glDeleteShader(frag);
	}
}

void GenericShader::bind() {
	ASSERT(prog);
	glUseProgram(prog);
	glEnableVertexAttribArray(aPosition);
	glEnableVertexAttribArray(aUV);
	glEnableVertexAttribArray(aColor);
}

void GenericShader::setCanvas(GLuint mvp, vec2 canvasSize, vec2 canvasOffset) {
	float zfar = 128;
	float znear = -128;
	float fan = zfar + znear;
	float fsn = zfar - znear;
	GLfloat orth[16] = {
		2.f/canvasSize.x, 0, 0, 0,
		0, -2.f/canvasSize.y, 0, 0,
		0, 0, 2.f/fsn, 0,
		-1+canvasOffset.x, 1-canvasOffset.y, -fan/fsn, 1
	};
	glUniformMatrix4fv(mvp, 1, 0, orth);
}

void GenericShader::setVertexBuffer(GenericVertex *buf) {
	const uint8_t *bytes = (uint8_t*) buf;
	glVertexAttribPointer(aPosition, 2, GL_FLOAT, GL_FALSE, sizeof(GenericVertex), bytes);
	glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, sizeof(GenericVertex), bytes+8);
	glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GenericVertex), bytes+16);	
}

void GenericShader::unbind() {
	glDisableVertexAttribArray(aPosition);
	glDisableVertexAttribArray(aUV);
	glDisableVertexAttribArray(aColor);
	glUseProgram(0);
}
