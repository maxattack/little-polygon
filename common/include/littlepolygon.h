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

#pragma once

// determine platform
#if __IPHONEOS__
#define LITTLE_POLYGON_MOBILE     1
#else
#define LITTLE_POLYGON_MOBILE     0
#endif

// standard includes
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <cstring>
#include <stdint.h>
#include <limits.h>
#include <SDL.h>
#if !LITTLE_POLYGON_MOBILE
#define GLEW_STATIC
#include <glew.h>
#include <SDL_opengl.h>
#else
#include <SDL_opengles2.h>
#endif
#include <SDL2/SDL_mixer.h>

//--------------------------------------------------------------------------------
// COMMON MACROS
//--------------------------------------------------------------------------------

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(_x)  ((void)sizeof(char[1 - 2*!(_x)]))
#endif
#ifndef arraysize
#define arraysize(a)   (sizeof(a) / sizeof((a)[0]))
#endif
#ifndef offsetof
#define offsetof(t,m)  ((uintptr_t)(uint8_t*)&(((t*)0)->m))
#endif
#ifndef MIN
#define MIN(a,b)   ((a) < (b) ? (a) : (b))
#define MAX(a,b)   ((a) > (b) ? (a) : (b))
#endif

//--------------------------------------------------------------------------------
// CONFIGURABLE MEMORY ALLOCATOR
//--------------------------------------------------------------------------------

#ifndef LPMALLOC
#define LPMALLOC malloc
#define LPFREE   free
#endif

//--------------------------------------------------------------------------------
// INTERNAL LOGGING
//--------------------------------------------------------------------------------

#ifdef DEBUG
#   define ASSERT(cond)     (assert(cond))
#	define CHECK(cond)      {int _result=(cond); ASSERT(_result);}
#   define LOG(_x)          printf _x
#   define LOG_MSG(_msg)    printf("%s:%d " _msg "\n", __FILE__, __LINE__)
#	define LOG_INT(_expr)	printf("%s:%d " #_expr " = %d\n", __FILE__, __LINE__, (_expr))
#	define LOG_FLOAT(_expr)	printf("%s:%d " #_expr " = %f\n", __FILE__, __LINE__, (_expr))
#	define LOG_VEC(_expr)	{ vec2 __u__ = (_expr); printf("%s:%d " #_expr " = <%f,%f>\n", __FILE__, __LINE__, __u__.x, __u__.y); }
#else
#   define ASSERT(cond)
#   define CHECK(cond)      {if (!(cond)) { puts("FATAL: ##cond"); exit(-1); }}
#   define LOG(_x)
#   define LOG_MSG(_msg)
#	define LOG_INT(_expr)
#	define LOG_FLOAT(_expr)
#	define LOG_VEC(_expr)
#endif

//------------------------------------------------------------------------------
// CONSTANTS
//------------------------------------------------------------------------------

#define ASCII_BEGIN          32
#define ASCII_END            127

#define TEXTURE_FLAG_FILTER  0x1
#define TEXTURE_FLAG_REPEAT  0x2
#define TEXTURE_FLAG_LUM     0x4
#define TEXTURE_FLAG_RGB     0x8

#define ASSET_TYPE_UNDEFINED 0
#define ASSET_TYPE_TEXTURE   1
#define ASSET_TYPE_IMAGE     2
#define ASSET_TYPE_FONT      3
#define ASSET_TYPE_SAMPLE    4
#define ASSET_TYPE_TILEMAP   5
#define ASSET_TYPE_USERDATA  6

#define CHANNEL_COUNT        16
#define SAMPLE_CAPACITY      64

//------------------------------------------------------------------------------
// ASSET RECORDS
// These records are directly mapped into memory from the asset binary.
//------------------------------------------------------------------------------

struct TextureAsset {
	void *compressedData; 	// zlib compressed texture data
	int32_t w, h; 	        // size of the texture (guarenteed to be POT)
	uint32_t 
		compressedSize,     // size of the compressed buffer, in bytes
		textureHandle,      // handle to the initialized texture resource
		flags;              // extra information (wrapping, format, etc)
	
	inline bool initialized() const { return textureHandle != 0; }
	inline int format() const { return GL_RGBA; }

	// if the texture is not initialized, decompress the it from memory
	// and transfer it to the graphics device
	void init();

	// if the texture is initialized, unload it from the graphics device
	void release();

	// bind the texture to the current graphics state, load lazily if needed
	void bind();
};

struct FrameAsset {
	float u0, v0,  // UV coordinates of the corners (necessary since the image
		u1, v1,    // might have been transposed during packing)
		u2, v2, 
		u3, v3;

	int32_t px, py, // post-trim pivot of the frame
		w, h;	    // post-trim size of the frame
};

struct ImageAsset {
	TextureAsset *texture; // the texture onto which this image is packed
	int32_t w, h,          // the logical size of the image
		px, py,            // the logical pivot of the image
		nframes;           // number of rendered frames

	inline FrameAsset* frame(int i) {
		// frames are stored immediately after the image (correctly aligned)
		ASSERT(i >= 0 && i < nframes);
		return reinterpret_cast<FrameAsset*>(this+1) + i;
	}	
};

struct uint8_pair_t {
	uint8_t x;
	uint8_t y;
};

struct TilemapAsset {
	uint8_pair_t *data;
	void *compressedData;   // zlib compressed tilemap buffer
	uint32_t tw, th,        // the size of the individual tiles
		mw, mh,             // the size of the tilemap
		compressedSize;     // the byte-length of the compressed buffer
	TextureAsset tileAtlas; // a texture-atlas of all the tiles

	inline bool intialized() const { return data != 0; }

	// if not initialized, decompress tiles to a dynamically-allocated buffer,
	// and transfer the tile atlas to the graphics device
	void init();

	// if initialized, free the dynamically-allocated buffer and release the tile
	// atlas from the graphics device
	void release();

	inline uint8_pair_t tileAt(int x, int y) const {
		ASSERT(intialized());
		ASSERT(x >= 0 && x < mw);
		ASSERT(y >= 0 && y < mh);
		return data[y * mw + x];
	}
};

struct GlyphAsset {
	int32_t x, y, // texel position of this glyph
		advance;  // pixel offset to the next character
};

struct FontAsset {
	int32_t height;                           // line-height of the font
	GlyphAsset glyphs[ASCII_END-ASCII_BEGIN]; // individual glyph metrics
	TextureAsset texture;                     // character texture-atlas
	
	GlyphAsset getGlyph(const char c) {
		// glyphs are stored in ASCII-order
		ASSERT(c >= ASCII_BEGIN && c < ASCII_END);
		return glyphs[c-ASCII_BEGIN];
	}

	const char* measureLine(const char *msg, int *outLength) {
		// measure the given line's total advance without rendering
		*outLength = 0;
		while(*msg && *msg != '\n') {
			*outLength += getGlyph(*msg).advance;
			++msg;
		}
		return msg;
	}	
};

struct SampleAsset {
	Mix_Chunk *chunk;              // initialized as a sdl mixer "chunk"
	void *compressedData;          // compressed PCM buffer
	int32_t channelCount,          // PCM stereo or mono?
		sampleWidth,               // PCM bits per sample
		frequency;                 // PCM samples per second
	uint32_t size, compressedSize; // byte-length of the compressed data

	inline bool initialized() const { return chunk != 0; }

	// if not initialized, decompress the PCM and load it into the audio device
	void init();

	// if initialized, unload the PCM data from the audio device
	void release();

	// play the sample, initializing lazily if necessary
	void play();
};

struct UserdataAsset {
	// A slice of plain old data that can represent anything - structured game assets,
	// UTF8 strings, etc.  Data immediately follows the initial size term.

	size_t size; // length of the data, in bytes

	inline void *data() const { return (void*)(this+1); }

	template<typename T> 
	T get() const { ASSERT(size == sizeof(T)); return *((T*)(this+1)); }
};

//------------------------------------------------------------------------------
// MAIN INTERFACE
//------------------------------------------------------------------------------

class AssetBundle {
public:
	
	AssetBundle() : assetCount(0), headers(0) {}
	~AssetBundle() { unload(); }

	// Load the asset bundle binary at the given sdl-managed path.
	// Returns null if there are any problems or if a crc is specified
	// which does not match the binary (before pointer fixup).
	int load(const char *path, uint32_t crc=0);

	// unload dynamically allocated memory, releasing assets first
	// if necessary
	void unload();

	// Load all the textures, audio samples, etc from their compressed
	// buffers into a ready-to-use form.
	void init();

	// Release all the textures, audio samples, etc that are in use.
	void release();

	// Assets are keyed by name-hashes (fnv-1a)
	// inlined so that the compile can constant-fold over string literals :)
	inline static uint32_t hash(const char* name) {
	    uint32_t hval = 0x811c9dc5;
	    while(*name) {
	        hval ^= (*name);
	        hval *= 0x01000193;
	        ++name;
	    }
	    return hval;
	}

	// lookup assets by name
	inline TextureAsset *texture(const char * name) { return texture(hash(name)); }
	inline ImageAsset *image(const char * name) { return image(hash(name)); }
	inline TilemapAsset *tilemap(const char * name) { return tilemap(hash(name)); }
	inline FontAsset *font(const char * name) { return font(hash(name)); }
	inline SampleAsset *sample(const char * name) { return sample(hash(name)); }
	inline UserdataAsset *userdata(const char *name) { return userdata(hash(name)); }

	// lookup assets by hash
	inline TextureAsset *texture(uint32_t hash) { return (TextureAsset*) findHeader(hash, ASSET_TYPE_TEXTURE); }
	inline ImageAsset *image(uint32_t hash) { return (ImageAsset*) findHeader(hash, ASSET_TYPE_IMAGE); }
	inline TilemapAsset *tilemap(uint32_t hash) { return (TilemapAsset*) findHeader(hash, ASSET_TYPE_TILEMAP); }
	inline FontAsset *font(uint32_t hash) { return (FontAsset*) findHeader(hash, ASSET_TYPE_FONT); }
	inline SampleAsset *sample(uint32_t hash) { return (SampleAsset*) findHeader(hash, ASSET_TYPE_SAMPLE); }
	inline UserdataAsset *userdata(uint32_t hash) { return (UserdataAsset*) findHeader(hash, ASSET_TYPE_USERDATA); }

private:

	// key->record mapping is done using a sorted array of hashes, with type double-checking.
	struct Header {
		uint32_t hash, type;
		void* data;
	};

	// number of assets
	int assetCount;

	// sorted dictionary array
	Header *headers;

	// key search and typecheck
	void* findHeader(uint32_t hash, uint32_t assetType);

};

