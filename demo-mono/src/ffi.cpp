#include <littlepolygon/context.h>

extern "C" {

int CreateContext(const char* caption, int w, int h, const char* assetPath) {
	LPInit(caption, w, h, assetPath);
	glClearColor(0.5, 0.6, 0.8, 0.0);	
	return 0;
}

int DestroyContext() {
	LPDestroy();
	return 0;
}

void ClearScreen() {
	glClear(GL_COLOR_BUFFER_BIT);
}

}
