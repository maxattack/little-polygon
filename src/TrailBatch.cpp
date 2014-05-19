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

#include <littlepolygon/trail.h>

GLchar TRAIL_SHADER[] = R"GLSL(
varying mediump vec4 color;

#if VERTEX

uniform highp mat4 mvp;
uniform mediump vec4 uColor;
uniform highp float uTime;
uniform highp float uFadeDurationInv;
attribute mediump vec3 aPosition;
attribute mediump float aTime;

void main() {
	gl_Position = mvp * vec4(aPosition, 1.0);
	color = vec4(uColor.xyz, uColor.w * (1.0 - (uTime - aTime) * uFadeDurationInv));
}

#else

void main() {
	gl_FragColor = color;
}

#endif
)GLSL";

// Shared shader handles (global variables == bad?)
static int globalRefCount = 0;

static GLuint prog;
static GLuint vert;
static GLuint frag;

static GLuint mvp;
static GLuint uColor;
static GLuint uTime;
static GLuint uFadeDurationInv;

static GLuint aPosition;
static GLuint aTime;

struct TrailBatch {
	// Vertices are appended to the trail using a circle-buffer.
	// If we overflow capacity, the we simply overwrite the oldest
	// vertex.  Note that because we render the trail as a strip,
	// there will be two vertices per particle.
	GLuint vbuf;
	
	int capacity;
	int count;
	int first;
	
	Color color;
	float stroke;
	float fadeDuration;
	float fadeDurationInv;
	float time;
	
	bool empty;
	vec2 prevPosition;

	struct Vertex {
		vec2 position;
		float z;
		float time;
	};
	
	int logicalToRaw(int i) {
		ASSERT(i >= 0 && i < count);
		return (first + i) % capacity;
	}
	
	float firstTime;
	
	float *times() { return &firstTime; }
	float setTime(int i, float t) { return times()[i>>1] = t; }
	float getTime(int i) { return times()[logicalToRaw(i)>>1]; }
	
	bool wrapping() const { return first + count > capacity; }
};

TrailBatch* createTrailBatch(size_t capacity) {
	auto result = (TrailBatch*) LITTLE_POLYGON_MALLOC(
		sizeof(TrailBatch) + capacity * sizeof(float)
	);
	
	if (globalRefCount == 0) {
		CHECK(compileShader(TRAIL_SHADER, &prog, &vert, &frag));
		mvp = glGetUniformLocation(prog, "mvp");
		uColor = glGetUniformLocation(prog, "uColor");
		uTime = glGetUniformLocation(prog, "uTime");
		uFadeDurationInv = glGetUniformLocation(prog, "uFadeDurationInv");
		
		aPosition = glGetAttribLocation(prog, "aPosition");
		aTime = glGetAttribLocation(prog, "aTime");
	}
	globalRefCount += 1;

	glGenBuffers(1, &result->vbuf); // assumes that ebuf is next

	size_t len = sizeof(TrailBatch::Vertex) * (2*capacity+2);
	glBindBuffer(GL_ARRAY_BUFFER, result->vbuf);
	glBufferData(GL_ARRAY_BUFFER, len, 0, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	
	result->capacity = 2*capacity; // two vertices per particle
	result->count = 0;
	result->first = 0;
	
	result->color = rgb(0xffffff);
	result->stroke = 1;
	result->time = 0;
	result->fadeDuration = 1;
	result->fadeDurationInv = 1;
	
	result->empty = true;
	
	return result;
}

void TrailBatchRef::destroy() {
	if (context) {
		globalRefCount -= 1;
		if (globalRefCount == 0) {
			glDeleteProgram(prog);
			glDeleteShader(frag);
			glDeleteShader(vert);
		}
		if (context) {
			glDeleteBuffers(2, &context->vbuf);
			LITTLE_POLYGON_FREE(context);
			context = 0;
		}
	}
}


Color TrailBatchRef::color() const {
	return context->color;
}

float TrailBatchRef::stroke() const {
	return context->stroke;
}

float TrailBatchRef::time() const {
	return context->time;
}

float TrailBatchRef::fadeDuration() const {
	return context->fadeDuration;
}

void TrailBatchRef::setColor(Color c) {
	context->color = c;
}

void TrailBatchRef::setStroke(float w) {
	context->stroke = w;
}

void TrailBatchRef::setFadeDuration(float seconds) {
	context->fadeDuration = seconds;
	context->fadeDurationInv = 1.0 / seconds;
}

void TrailBatchRef::clear() {
	if (context->count > 0) {
		context->count = 0;
		context->first = 0;
		context->empty = 1;
	}
}

void TrailBatchRef::append(vec2 position, float z, float stroke, float minDist) {
	if (context->count == context->capacity) {
		
		// pop the oldest particles
		context->first = (context->first + 2) % context->capacity;
		context->count -= 2;
		
	}
	
	if (context->empty) {
		
		// don't have enough info to actually plot anything
		context->empty = 0;
		context->prevPosition = position;
		
	} else {
		
		vec2 offset = position - context->prevPosition;
		if (offset.norm() < minDist * minDist) {
			// bail early if there isn't too much distance
			return;
		} else {
			context->prevPosition = position;
		}
		
		int i = context->count;
		context->count += 2;
		auto rawStroke = stroke * context->stroke;
		vec2 unit = rawStroke * offset.normalized().clockwise();
		
		TrailBatch::Vertex v0[2];
		
		v0[0].position = position + unit;
		v0[0].z = z;
		v0[0].time = context->time;
		v0[1].position = position - unit;
		v0[1].time = context->time;
		v0[1].z = z;
		
		int ri = context->logicalToRaw(i);
		context->setTime(ri, context->time);
		
		glBindBuffer(GL_ARRAY_BUFFER, context->vbuf);
		glBufferSubData(
			GL_ARRAY_BUFFER,
			(ri + 2) * sizeof(TrailBatch::Vertex),
			2*sizeof(TrailBatch::Vertex),
			v0
		);
		
		// special case - the last vertex needs to be "doubled up"
		if (ri+2 == context->capacity) {
			glBufferSubData(GL_ARRAY_BUFFER, 0, 2*sizeof(TrailBatch::Vertex), v0);
		}
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
	}
	
	
}

void TrailBatchRef::tick(float deltaSeconds) {
	if (context->count > 0) {
		context->time += deltaSeconds;
		
		int removeCount = 0;
		while(
			removeCount < context->count &&
			context->getTime(removeCount) + fadeDuration() < context->time
		) {
			removeCount+=2;
		}
		
		if (removeCount > 0) {
			context->first = (context->first + removeCount) % context->capacity;
			context->count -= removeCount;
		}
		
		if (context->count == 0) {
			context->empty = 1;
			context->time = 0; // reset the clock to try and avoid unbounded time
			// (TODO: a continuous trail won't be able to deal with this, so
			// we may need to periodically reset the clock and refresh the whole
			// vertex buffer -- every few minutes or so?)
		}
		
	}
}

void TrailBatchRef::draw(const Viewport &view) {
	// assumes that blending is already enabled if the application
	// wants it
	if (context->count > 2) {
		glUseProgram(prog);
		glBindBuffer(GL_ARRAY_BUFFER, context->vbuf);
		
		view.setMVP(mvp);
		glUniform4f(
			uColor,
			context->color.red(),
			context->color.green(),
			context->color.blue(),
			context->color.alpha()
		);
		glUniform1f(uTime, context->time);
		glUniform1f(uFadeDurationInv, context->fadeDurationInv);
		
		glEnableVertexAttribArray(aPosition);
		glEnableVertexAttribArray(aTime);
		glVertexAttribPointer(aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(TrailBatch::Vertex), 0);
		glVertexAttribPointer(aTime, 1, GL_FLOAT, GL_FALSE, sizeof(TrailBatch::Vertex), (void*)(3*sizeof(float)));
		
		// if we're currently wrapping around then we need to make two calls (being sure to
		// omit two-vertex degenerate cases), otherwise it's just a simple single call
		// (remember to add 2 to "first" to account for the dup vertices)
		if (context->wrapping()) {
			int firstLength = context->capacity - context->first;
			if (firstLength > 2) {
				glDrawArrays(GL_TRIANGLE_STRIP, context->first+2, firstLength);
			}
			if (context->count - firstLength > 0) {
				glDrawArrays(GL_TRIANGLE_STRIP, 0, context->count - firstLength+2);
			}
		} else {
			glDrawArrays(GL_TRIANGLE_STRIP, context->first+2, context->count);
		}
		glDisableVertexAttribArray(aPosition);
		glDisableVertexAttribArray(aTime);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

