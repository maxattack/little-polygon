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
