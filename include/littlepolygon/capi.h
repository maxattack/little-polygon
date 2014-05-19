#ifndef LITTLEPOLYGON_CAPI__
#define LITTLEPOLYGON_CAPI__

#include <stdint.h>

#ifdef __cplusplus 
extern "C" {
#endif

// C-API - A laundry-list of plain-C "syscalls" for making it easy to include
// little polygon in any environment that supports FFI (LuaJIT, Python, etc)

typedef uint32_t result_t;

result_t littlepolygon_initialize(const char* caption, int w, int h);
result_t littlepolygon_destroy();


#ifdef __cplusplus 
}
#endif
#endif
