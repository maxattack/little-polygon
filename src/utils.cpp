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
#include <algorithm>

bool linearIntersection(vec2 u0, vec2 u1, vec2 v0, vec2 v1, float& u) {
	float norm = (v1.y - v0.y)*(u1.x-u0.x) - (v1.x-v0.x)*(u1.y-u0.y);
	if (norm > -M_COLINEAR_SLOP && norm < M_COLINEAR_SLOP) {
		// lines are parallel
		return false;
	}
	norm = 1.0f / norm;
	u = ((v1.x-v0.x)*(u0.y-v0.y) - (v1.y-v0.y)*(u0.x-v0.x)) * norm;
	return true;  
}

bool linearIntersection(vec2 u0, vec2 u1, vec2 v0, vec2 v1, float& u, float& v) {
	float norm = (v1.y - v0.y)*(u1.x-u0.x) - (v1.x-v0.x)*(u1.y-u0.y);
	if (norm > -M_COLINEAR_SLOP && norm < M_COLINEAR_SLOP) {
		// lines are parallel
		return false;
	}
	norm = 1.0f / norm;
	u = ((v1.x-v0.x)*(u0.y-v0.y) - (v1.y-v0.y)*(u0.x-v0.x)) * norm;
	v = ((u1.x-u0.x)*(u0.y-v0.y) - (u1.y-u0.y)*(u0.x-v0.x)) * norm;
	return true;  
}

vec2 quadraticBezier(vec2 p0, vec2 p1, vec2 p2, float u) {
	return ((1.0f-u)*(1.0f-u))*p0 + (2.0f*(1.0f-u)*u)*p1 + (u*u)*p2;
}
    
vec2 quadraticBezierDeriv(vec2 p0, vec2 p1, vec2 p2, float u) {
	return (2.0f*(1.0f-u))*(p1-p0) + (2.0f*u)*(p2-p1);
}

vec2 cubicBezier(vec2 p0, vec2 p1, vec2 p2, vec2 p3, float u) {
	return ((1.0f-u) * (1.0f-u) * (1.0f-u)) * p0 +
		(3.0f * (1.0f-u) * (1.0f-u) * u) * p1 +
		(3.0f * (1.0f-u) * u * u) * p2 +
		(u * u * u) * p3;
}

vec2 cubicBezierDeriv(vec2 p0, vec2 p1, vec2 p2, vec2 p3, float u){
	return 3.0f * (
		(-(1.0f-u) * (1.0f-u)) * p0 +
		(1.0f - 4.0f * u + 3.0f * u * u) * p1 +
		(2.0f * u - 3.0f * u * u) * p2 +
		(u * u) * p3
	);
}
vec2 cubicHermite(vec2 p0, vec2 m0, vec2 p1, vec2 m1, float u) {
	return (2.f*u*u*u - 3.f*u*u + 1.f) * p0 +
		(u*u*u - 2.f*u*u + u) * m0 +
		(-2.f*u*u*u + 3.f *u*u) * p1 +
		(u*u*u - u*u) * m1;
}

vec2 CubicHermiteDeriv(vec2 p0, vec2 m0, vec2 p1, vec2 m1, float u) {
	return (6.f*(u*u - u)) * p0 +
		(3.f*u*u - 4.f*u + 1.f) * m0 +
		(6.f*(u - u*u)) * p1 +
		(3.f*u*u - 2.f*u) * m1;
}

Color hsv(float h, float s, float v) {
	if(s > 0.001f) {
		h /= 60;
		int i = int(h);
		float f = h - i; // factorial part of h
		float p = v * ( 1 - s );
		float q = v * ( 1 - s * f );
		float t = v * ( 1 - s * ( 1 - f ) );
		switch( i ) {
			case 0: return rgb(v, t, p);
			case 1: return rgb(q, v, p);
			case 2: return rgb(p, v, t);
			case 3: return rgb(p, q, v);
			case 4: return rgb(t, p, v);
			default: return rgb(v, p, q);
		}
	} else {
		return rgb(v, v, v);
	}
}

void Color::toHSV(float *h, float *s, float *v) {
	float r = red();
	float g = green();
	float b = blue();
	float K = 0.f;
	if (g < b) {
		std::swap(g, b);
		K -= 1.f;
	}
	if (r < g) {
		std::swap(r, g);
		K = -2.f / 6.f - K;
	}

	float chroma = r - std::min(g, b);
	*h = 360.f * fabs(K + (g-b) / (6.f * chroma + 1e-20f));
	*s = chroma / (r + 1e-20f);
	*v = r;
}

GLuint generateTexture(TextureGenerator cb, int w, int h) {
	GLuint result;
	glGenTextures(1, &result);
	glBindTexture(GL_TEXTURE_2D, result);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	Color *scratch = (Color*) malloc( sizeof(Color) * w * h );
	double dx = 1.0 / (w-1.0);
	double dy = 1.0 / (h - 1.0);
	for(int y=0; y<h; ++y)
	for(int x=0; x<w; ++x) {
		scratch[x + y * w] = cb(x*dx, y*dy);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, scratch);	
	free(scratch);
	return result;
}

//int createRenderToTextureFramebuffer(GLsizei w, GLsizei h, GLuint *t, GLuint *f) {
//	glGenFramebuffers(1, f);
//	glGenTextures(1, t);
//	
//	glBindFramebuffer(GL_FRAMEBUFFER, *f);
//	glBindTexture(GL_TEXTURE_2D, *t);
//	
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *t, 0);
//	
////	GLuint depthBuffer;
////	glGenRenderbuffers(1, &depthBuffer);
////	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
////	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);
////	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
//	
//	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
//	if (status != GL_FRAMEBUFFER_COMPLETE) {
//		LOG(("Framebuffer Status: %x\n", (int)status));
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//		glDeleteFramebuffers(1, f);
//		glDeleteTextures(1, t);
//		return 1;
//	} else {
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//		return 0;
//	}
//	
//}
//
//
//
