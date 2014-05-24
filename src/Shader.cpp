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

Shader::Shader(const GLchar *vsrc, const GLchar *fsrc) {
	prog = glCreateProgram();
	vert = glCreateShader(GL_VERTEX_SHADER);
	frag = glCreateShader(GL_FRAGMENT_SHADER);

	// setup source and compile
	const char preamble[] = "\n";
	const char* vsrcList[] = { preamble, vsrc };
	const char* fsrcList[] = { preamble, fsrc };
	int vlengths[] = { (int) strlen(preamble), (int) strlen(vsrc) };
	int flengths[] = { (int) strlen(preamble), (int) strlen(fsrc) };
	glShaderSource(vert, 2, vsrcList, vlengths);
	glShaderSource(frag, 2, fsrcList, flengths);
	glCompileShader(vert);
	glCompileShader(frag);

	// check compile results
	GLint result;
	glGetShaderiv(vert, GL_COMPILE_STATUS, &result);
	if (result != GL_TRUE) {
		GLchar buf[256];
		int len;
		glGetShaderInfoLog(vert, 256, &len, buf);
		LOG(("VERTEX %s\n", buf));
		glDeleteProgram(prog);
		glDeleteShader(frag);
		glDeleteShader(vert);        
		prog = 0;
		frag = 0;
		vert = 0;
	}

	glGetShaderiv(frag, GL_COMPILE_STATUS, &result);
	if (result != GL_TRUE) {
		GLchar buf[256];
		int len;
		glGetShaderInfoLog(frag, 256, &len, buf);
		LOG(("FRAGMENT %s\n", buf));
		glDeleteProgram(prog);
		glDeleteShader(frag);
		glDeleteShader(vert);        
		prog = 0;
		frag = 0;
		vert = 0;
	}

	// link
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glBindFragDataLocation(prog, 0, "outColor");
	glLinkProgram(prog);
	

	// check link results
	glGetProgramiv(prog, GL_LINK_STATUS, &result);
	if (result != GL_TRUE) {
		GLchar buf[256];
		int len;
		glGetProgramInfoLog(prog, 256, &len, buf);
		LOG(("LINK %s\n", buf));
		glDeleteProgram(prog);
		glDeleteShader(frag);
		glDeleteShader(vert);        
		prog = 0;
		frag = 0;
		vert = 0;		
	}

}

Shader::~Shader() {
	if (prog) {
		glDeleteProgram(prog);
		glDeleteShader(vert);
		glDeleteShader(frag);
	}
}

