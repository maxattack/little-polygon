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
#include "littlepolygon_math.h"
#include "littlepolygon_graphics.h"
#include <algorithm>


#if LITTLE_POLYGON_MOBILE
static int handleAppEvents(void *userdata, SDL_Event *event) {
	switch (event->type) {
	case SDL_QUIT:
		exit(0);
		break;

	case SDL_APP_TERMINATING:
		// shut down everything
		return 0;
			
	case SDL_APP_LOWMEMORY:
		// release as much as possible?
		return 0;
			
	case SDL_APP_WILLENTERBACKGROUND:
		return 0;
			
	case SDL_APP_DIDENTERBACKGROUND:
		// 5s to save state or you are dead
		return 0;
			
	case SDL_APP_WILLENTERFOREGROUND:
		return 0;
			
	case SDL_APP_DIDENTERFOREGROUND:
		return 0;
			
	default:
		// just put event on the queue
		return 1;
	}
}
#endif

static void doTearDown() {
  Mix_CloseAudio();
  SDL_Quit();
}

SDL_Window *initContext(const char *caption, int w, int h) {
	#if LITTLE_POLYGON_MOBILE
	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
	#else
	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK);
	#endif
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024);
	atexit(doTearDown);

	#if LITTLE_POLYGON_MOBILE
		SDL_SetEventFilter(handleAppEvents, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);		
		SDL_Window *pWindow = SDL_CreateWindow(
			"", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0,
			SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN|SDL_WINDOW_BORDERLESS|SDL_WINDOW_SHOWN
		);

	#else

		#if LITTLE_POLYGON_GL_CORE_PROFILE
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		#endif
		if (w == 0) {
			// iphone 5 resolution :P
			w = 1136;
			h = 640;
		}
		SDL_Window *pWindow = SDL_CreateWindow(
			caption ? caption : "Little Polygon Context",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			w, h, SDL_WINDOW_OPENGL
		);

	#endif

	SDL_GL_CreateContext(pWindow);
	#if !LITTLE_POLYGON_MOBILE
		glewInit();
	#endif

	SDL_GetWindowSize(pWindow, &w, &h);
	glViewport(0, 0, w, h);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	#if !LITTLE_POLYGON_MOBILE
		glEnableClientState(GL_VERTEX_ARRAY);
	#endif
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return pWindow;
}

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

bool compileShader(const GLchar* source, GLuint *outProg, GLuint *outVert, GLuint *outFrag) {
	int prog = glCreateProgram();
	int vert = glCreateShader(GL_VERTEX_SHADER);
	int frag = glCreateShader(GL_FRAGMENT_SHADER);

	// setup source and compile
	const char vpreamble[] = "#define VERTEX 1\n";
	const char fpreamble[] = "#define VERTEX 0\n";
	int slen = (int) strlen(source);
	
	#if LITTLE_POLYGON_MOBILE
		const char* vsrcList[] = { vpreamble, source };
		const char* fsrcList[] = { fpreamble, source };
		int vlengths[] = { (int) strlen(vpreamble), slen };
		int flengths[] = { (int) strlen(fpreamble), slen };
		glShaderSource(vert, 2, vsrcList, vlengths);
		glShaderSource(frag, 2, fsrcList, flengths);
	#else
		const char preamble[] = "#version 120\n#define mediump  \n#define highp  \n#define lowp  \n";
		const char* vsrcList[] = { preamble, vpreamble, source };
		const char* fsrcList[] = { preamble, fpreamble, source };
		int vlengths[] = { (int) strlen(preamble), (int) strlen(vpreamble), slen };
		int flengths[] = { (int) strlen(preamble), (int) strlen(fpreamble), slen };
		glShaderSource(vert, 3, vsrcList, vlengths);
		glShaderSource(frag, 3, fsrcList, flengths);
	#endif
	
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
		return false;
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
		return false;
	}

	// link
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
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
		return false;
	}

	// save handles
	*outProg = prog;
	*outVert = vert;
	*outFrag = frag;  
	return true;
}

GLuint generateTexture(TextureGenerator cb, int w, int h) {
	GLuint result;
	glGenTextures(1, &result);
	glBindTexture(GL_TEXTURE_2D, result);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	Color *scratch = (Color*) LITTLE_POLYGON_MALLOC( sizeof(Color) * w * h );
	double dx = 1.0 / (w-1.0);
	double dy = 1.0 / (h - 1.0);
	for(int y=0; y<h; ++y)
	for(int x=0; x<w; ++x) {
		scratch[x + y * w] = cb(x*dx, y*dy);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, scratch);	
	LITTLE_POLYGON_FREE(scratch);
	return result;
}

GLuint getFakeAntialiasTexture() {
	static GLuint result = 0;
	if (!result) {
		result = generateTexture([](double x, double y) {
			double distance = 2*fabs(x-0.5);
			// might consider using a geometric function here to cover
			// a larger range?
			double threshold = 0.9 + 0.1 * sqrt(y);
			if(distance < threshold) {
				return rgb(0xffffff);
			} else {
				double u = (distance - threshold) / (1 - threshold);
				return rgba(rgb(0xffffff), 1 - u*u);
			}
		});
	}
	return result;
}

