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
#include "graphics.h"
#include "sprites.h"
#include "rig.h"

//------------------------------------------------------------------------------
// CONSTANTS

#define ASSET_TYPE_UNDEFINED 0
#define ASSET_TYPE_TEXTURE   1
#define ASSET_TYPE_IMAGE     2
#define ASSET_TYPE_FONT      3
#define ASSET_TYPE_SAMPLE    4
#define ASSET_TYPE_TILEMAP   5
#define ASSET_TYPE_PALETTE   6
#define ASSET_TYPE_USERDATA  7
#define ASSET_TYPE_RIG       8

//------------------------------------------------------------------------------
// GENERAL ASSETS

struct SampleAsset
{
	
	Mix_Chunk* chunk;           // initialized as a sdl mixer "chunk"
	void*      compressedData;  // compressed PCM buffer
	int32_t    channelCount,    // PCM stereo or mono?
			   sampleWidth,     // PCM bits per sample
			   frequency;       // PCM samples per second
	uint32_t   size,            // byte-length of the compressed data
	           compressedSize;

	bool initialized() const { return chunk != 0; }

	void init();
	void play();
	void release();
	
};

struct PaletteAsset
{
	
	uint32_t count;

	Color *colors() const { return (Color*)(this+1); }

	Color getColor(int i) const {
		ASSERT(i >= 0);
		ASSERT(i < count);
		return colors()[i];
	}
	
};

struct CompressedUserdata
{
	
	uint32_t size;
	uint32_t compressedSize;
	void*    compressedData;
	
	void inflate(void* result);
	
};


//------------------------------------------------------------------------------
// MAIN INTERFACE

struct AssetData;

class AssetBundle {
private:
	AssetData *data;
	AssetBundle *fallback;

public:
	AssetBundle(const char* path=0, uint32_t crc=0);
	~AssetBundle();

	#ifdef DEBUG
	#define ASSET_RESULT_VERIFY(expr) auto res = (expr); if (!res) { LOG(("ASSET UNDEFINED: %s\n", name)); } return res;
	#else
	#define ASSET_RESULT_VERIFY(expr) return expr;
	#endif

	// lookup assets by name
	TextureAsset *texture(const char *name) { ASSET_RESULT_VERIFY(texture(fnv1a(name))); }
	ImageAsset *image(const char *name) { ASSET_RESULT_VERIFY(image(fnv1a(name))); }
	TilemapAsset *tilemap(const char *name) { ASSET_RESULT_VERIFY(tilemap(fnv1a(name))); }
	FontAsset *font(const char *name) { ASSET_RESULT_VERIFY(font(fnv1a(name))); }
	SampleAsset *sample(const char *name) { ASSET_RESULT_VERIFY(sample(fnv1a(name))); }
	PaletteAsset *palette(const char *name) { ASSET_RESULT_VERIFY(palette(fnv1a(name))); }
	RigAsset *rig(const char *name) { ASSET_RESULT_VERIFY(rig(fnv1a(name))); }
	
	template<typename T>
	T *userdata(const char *name) { ASSET_RESULT_VERIFY(userdata<T>(fnv1a(name))) }

	
	#undef ASSET_RESULT_VERIFY

	// lookup assets by hash
	TextureAsset *texture(uint32_t hash) { return (TextureAsset*) findHeader(hash, ASSET_TYPE_TEXTURE); }
	ImageAsset *image(uint32_t hash) { return (ImageAsset*) findHeader(hash, ASSET_TYPE_IMAGE); }
	TilemapAsset *tilemap(uint32_t hash) { return (TilemapAsset*) findHeader(hash, ASSET_TYPE_TILEMAP); }
	FontAsset *font(uint32_t hash) { return (FontAsset*) findHeader(hash, ASSET_TYPE_FONT); }
	SampleAsset *sample(uint32_t hash) { return (SampleAsset*) findHeader(hash, ASSET_TYPE_SAMPLE); }
	PaletteAsset *palette(uint32_t hash) { return (PaletteAsset*) findHeader(hash, ASSET_TYPE_PALETTE); }
	RigAsset *rig(uint32_t hash) { return (RigAsset*) findHeader(hash, ASSET_TYPE_RIG); }
	
	template<typename T>
	T *userdata(uint32_t hash) { return (T*) findHeader(hash, ASSET_TYPE_USERDATA); }

	// headers are sorted by hash, so lookup is LOG(N)
	void* findHeader(uint32_t hash, uint32_t assetType);
	void setFallback(AssetBundle* fallback);

	// by default assets are initialized lazily, but this method will eagerly initialize
	// the whole asset data;
	void init();

	// release all intialized assets, but don't free the POD from memory
	void release();

};


