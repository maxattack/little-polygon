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

#include "littlepolygon/utils.h"
#include "littlepolygon/math.h"
#include "littlepolygon/graphics.h"

// TODO: Handle Window Resize?

PostProcessingFX::PostProcessingFX(const GLchar *source) : dfb(0), fb(0) {
#if LITTLE_POLYGON_MOBILE
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&dfb);
#endif
	if (source) {
		int w, h;
		SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &w, &h);
		if (createRenderToTextureFramebuffer(w, h, &rt, &fb)) {
			return;
		}
		if (!compileShader(source, &prog, &vsh, &fsh)) {
			glDeleteFramebuffers(1, &fb);
			glDeleteTextures(1, &rt);
			fb = 0;
			rt = 0;
			return;
		}
		ap = glGetAttribLocation(prog, "aPosition");
		auv = glGetAttribLocation(prog, "aUv");
		glGenBuffers(1, &vbuf);
		glBindBuffer(GL_ARRAY_BUFFER, vbuf);
		vec2 varr[8] = {
			vec(-1,-1), vec(0,0),
			vec(-1,1), vec(0,1),
			vec(1,-1), vec(1,0),
			vec(1,1), vec(1,1)
		};
		glBufferData(GL_ARRAY_BUFFER, 8*sizeof(vec2), varr, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, dfb);
	}
}

PostProcessingFX::~PostProcessingFX() {
	release();
}

void PostProcessingFX::release() {
	if (valid()) {
		glDeleteFramebuffers(1, &fb);
		glDeleteTextures(1, &rt);
		glDeleteBuffers(1, &vbuf);
		glDeleteShader(vsh);
		glDeleteShader(fsh);
		glDeleteProgram(prog);
		fb = 0;
	}
}

void PostProcessingFX::beginScene() {
	if (fb) {
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
	}
}

void PostProcessingFX::endScene() {
	if (fb) {
		glBindFramebuffer(GL_FRAMEBUFFER, dfb);
	}
}

void PostProcessingFX::draw() {
	if (fb) {
		glUseProgram(prog);
		glEnableVertexAttribArray(ap);
		glEnableVertexAttribArray(auv);
		glBindBuffer(GL_ARRAY_BUFFER, vbuf);
		glVertexAttribPointer(ap, 2, GL_FLOAT, GL_FALSE, 2*sizeof(vec2), 0);
		glVertexAttribPointer(auv, 2, GL_FLOAT, GL_FALSE, 2*sizeof(vec2), (GLfloat*)sizeof(vec2));
		glBindTexture(GL_TEXTURE_2D, rt);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDisableVertexAttribArray(ap);
		glDisableVertexAttribArray(auv);
	}
}
