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

#include <littlepolygon_trail.h>

GLchar TRAIL_SHADER[] = R"GLSL(
varying mediump vec4 color;

#if VERTEX

uniform highp mat4 mvp;
uniform mediump vec4 uColor;
uniform highp float uTime;
uniform highp float uFadeDurationInv;
attribute mediump vec2 aPosition;
attribute mediump float aTime;

void main() {
	gl_Position = mvp * vec4(aPosition, 0.0, 1.0);
	color = vec4(uColor.xyz, uColor.w * (1.0 - (uTime - aTime) * uFadeDurationInv));
}

#else

void main() {
	gl_FragColor = color;
}

#endif
)GLSL";


struct TrailBatch {
	// OpenGL handles
	GLuint prog;
	GLuint vert;
	GLuint frag;
	
	GLuint mvp;
	GLuint uColor;
	GLuint uTime;
	GLuint uFadeDurationInv;
	
	GLuint aPosition;
	GLuint aTime;
	
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
		float time;
	};
	
	// we don't actually use these, but it makes bookkeeping easier :P
	Vertex dup1, dup2;
	Vertex *rawBuffer() { return &dup1; }
	
	Vertex *vertexBuffer() { return (&dup2)+1; }
	
	Vertex *rawVertex(int i) {
		ASSERT(i >= 0 && i < capacity);
		// the actual "headVertex" is a special "extra" dup
		// of the last vertex
		return vertexBuffer() + i;
	}
	
	Vertex *tail() {
		return rawVertex(capacity-1);
	}
	
	int logicalToRaw(int i) {
		ASSERT(i >= 0 && i < count);
		return (first + i) % capacity;
	}
	
	Vertex *logicalVertex(int i) {
		return rawVertex(logicalToRaw(i));
	}
	
	bool wrapping() const { return first + count > capacity; }
};

TrailBatch* createTrailBatch(size_t capacity) {
	auto result = (TrailBatch*) LITTLE_POLYGON_MALLOC(
		sizeof(TrailBatch) +
		(2 * capacity) * sizeof(TrailBatch::Vertex)
	);
	
	CHECK(
		compileShader(TRAIL_SHADER, &result->prog, &result->vert, &result->frag)
	);
	
	result->mvp = glGetUniformLocation(result->prog, "mvp");
	result->uColor = glGetUniformLocation(result->prog, "uColor");
	result->uTime = glGetUniformLocation(result->prog, "uTime");
	result->uFadeDurationInv = glGetUniformLocation(result->prog, "uFadeDurationInv");
	
	result->aPosition = glGetAttribLocation(result->prog, "aPosition");
	result->aTime = glGetAttribLocation(result->prog, "aTime");
	
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
		glDeleteBuffers(2, &context->vbuf);
		glDeleteProgram(context->prog);
		glDeleteShader(context->frag);
		glDeleteShader(context->vert);
		LITTLE_POLYGON_FREE(context);
		context = 0;
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

void TrailBatchRef::append(vec2 position, float stroke) {
	if (context->count == context->capacity) {
		// TODO: something better than NOOP :P
		return;
	}
	
	if (context->empty) {
		
		// don't have enough info to actually plot anything
		context->empty = 0;
		context->prevPosition = position;
		
	} else {
		
		vec2 offset = position - context->prevPosition;
		if (offset.norm() < 0.1 * 0.1) {
			// bail early if there isn't too much distance
			return;
		} else {
			context->prevPosition = position;
		}
		
		int i = context->count;
		context->count += 2;
		auto rawStroke = stroke * context->stroke;
		vec2 unit = rawStroke * offset.normalized().clockwise();

		auto v0 = context->logicalVertex(i);
		v0->position = position + unit;
		v0->time = context->time;
		
		// this is OK because pairs of vertices will not fall on the
		// cycle-boundary
		v0[1].position = position - unit;
		v0[1].time = context->time;
		
		int ri = context->logicalToRaw(i);
		
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
			context->logicalVertex(removeCount)->time + fadeDuration() < context->time
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
		glUseProgram(context->prog);
		glBindBuffer(GL_ARRAY_BUFFER, context->vbuf);
		
		view.setMVP(context->mvp);
		glUniform4f(
			context->uColor,
			context->color.red(),
			context->color.green(),
			context->color.blue(),
			context->color.alpha()
		);
		glUniform1f(context->uTime, context->time);
		glUniform1f(context->uFadeDurationInv, context->fadeDurationInv);
		
		glEnableVertexAttribArray(context->aPosition);
		glEnableVertexAttribArray(context->aTime);
		glVertexAttribPointer(context->aPosition, 2, GL_FLOAT, GL_FALSE, sizeof(TrailBatch::Vertex), 0);
		glVertexAttribPointer(context->aTime, 1, GL_FLOAT, GL_FALSE, sizeof(TrailBatch::Vertex), (void*)sizeof(vec2));
		
		// if we're currently wrapping around then we need to make two calls (being sure to
		// omit two-vertex degenerate cases), otherwise it's just a simple single call
		// (remember to add 2 to "first" to account for the dup vertices)
		if (context->wrapping()) {
			int firstLength = context->capacity - context->first;
			if (firstLength > 2) {
				glDrawArrays(GL_TRIANGLE_STRIP, context->first+2, firstLength);
			}
			if (context->count - firstLength > 2) {
				glDrawArrays(GL_TRIANGLE_STRIP, 0, context->count - firstLength);
			}
		} else {
			glDrawArrays(GL_TRIANGLE_STRIP, context->first+2, context->count);
		}
		glDisableVertexAttribArray(context->aPosition);
		glDisableVertexAttribArray(context->aTime);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

