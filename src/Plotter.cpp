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
#include <littlepolygon/graphics.h>

Plotter::Plotter(int cap) :
capacity(cap),
currentArray(0) {
	vertices = (Vertex*) malloc(sizeof(Vertex) * capacity);
	glGenBuffers(3, vbo);
	
	for(int i=0; i<3; ++i) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[i]);
		glBufferData(GL_ARRAY_BUFFER, capacity*sizeof(Vertex), 0, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Plotter::~Plotter() {
	free(vertices);
	glDeleteBuffers(3, vbo);
}

void Plotter::swapBuffer() {
	currentArray=(currentArray+1) % 3;
}

void Plotter::bufferData(int count) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo[currentArray]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, count*sizeof(Vertex), vertices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
