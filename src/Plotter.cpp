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
