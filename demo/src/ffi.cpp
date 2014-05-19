#include <littlepolygon/context.h>
#include <littlepolygon/ffi.h>

extern "C" int littlepolygon_initialize(const char* caption, int w, int h, const char* assetPath) {
	LPInit(caption, w, h, assetPath);
	return 0;
}

extern "C" int littlepolygon_destroy() {
	LPDestroy();
	return 0;
}

extern "C" void littlepolygon_clearscreen() {
	glClear(GL_COLOR_BUFFER_BIT);
}

