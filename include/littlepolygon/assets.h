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
#include "base.h"
#include "math.h"

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
#define ASSET_TYPE_PALETTE   6
#define ASSET_TYPE_USERDATA  7
#define ASSET_TYPE_RIG       8

//------------------------------------------------------------------------------
// ASSET RECORDS
// These records are directly mapped into memory from the asset binary.  We 
// should be able to construct these procedurally as well, though I haven't run
// up against that use case yet in practice.
//------------------------------------------------------------------------------

struct TextureAsset
{

	void*    compressedData;  // zlib compressed texture data
	int32_t  w, h;            // size of the texture (guarenteed to be POT)
	uint32_t compressedSize,  // size of the compressed buffer, in bytes
	         textureHandle,   // handle to the initialized texture resource
	         flags;           // extra information (wrapping, format, etc)
	
	bool initialized() const { return textureHandle != 0; }
	int format() const { return GL_RGBA; }
	Vec2 size() const { return vec((float)w,(float)h); }

	void init();
	void bind();
	void release();
	
};

//------------------------------------------------------------------------------

struct FrameAsset
{
	
	Vec2 uv0, uv1,  // UV coordinates of the corners (necessary since the image
	     uv2, uv3,  // might have been transposed during packing)
	     pivot,    // post-trim pivot of the frame
	     size;      // post-trim size of the frame

	
};

struct ImageAsset
{
	
	TextureAsset* texture; // the texture onto which this image is packed
	Vec2 size, pivot;
	uint32_t nframes;

	FrameAsset* frame(int i) {
		ASSERT(i >= 0 && i < nframes);
		return reinterpret_cast<FrameAsset*>(this+1) + i;
	}
	
};

//------------------------------------------------------------------------------

struct TileAsset
{
	
	uint8_t x, y; // Atlas Cell Location
	
	bool isDefined() const { return x != 0xff; }
	
};

struct TilemapAsset
{
	
	TileAsset*   data;           // NULL when uninitialized
	void*        compressedData; // zlib compressed tilemap buffer
	uint32_t     tw, th,         // the size of the individual tiles
	             mw, mh,         // the size of the tilemap
	             compressedSize; // the byte-length of the compressed buffer
	TextureAsset tileAtlas;      // a texture-atlas of all the tiles

	bool initialized() const { return data != 0; }
	Vec2 tileSize() const { return vec((float)tw,(float)th); }
	Vec2 mapSize() const { return vec((float)mw,(float)mh); }
	TileAsset tileAt(int x, int y) const;
	
	void init();
	void release();
	
	void reload();
	void clearTile(int x, int y);
	
};

//------------------------------------------------------------------------------

struct GlyphAsset
{
	
	int32_t x, y,     // texel position of this glyph
	        advance;  // pixel offset to the next character
	
};

struct FontAsset
{
	
	int32_t      height;                        // line-height of the font
	GlyphAsset   glyphs[ASCII_END-ASCII_BEGIN]; // individual glyph metrics
	TextureAsset texture;                       // character texture-atlas
	
	GlyphAsset getGlyph(const char c) {
		ASSERT(c >= ASCII_BEGIN && c < ASCII_END);
		return glyphs[c-ASCII_BEGIN];
	}

	const char* measureLine(const char *msg, int *outLength) {
		*outLength = 0;
		while(*msg && *msg != '\n') {
			*outLength += getGlyph(*msg).advance;
			++msg;
		}
		return msg;
	}
	
};

//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------

struct RawUserdata
{
	
	uint32_t size;
	
	void *data() const { return (void*)(this+1); }

	template<typename T>
	const T* as() { return (T*)(this+1); }
	
	template<typename T>
	const T& get() { ASSERT(size == sizeof(T)); return *((T*)(this+1)); }
	
};

struct CompressedUserdata
{
	
	uint32_t size;
	uint32_t compressedSize;
	void*    compressedData;
	
	void inflate(void* result);
	
};

//------------------------------------------------------------------------------

// TODO: EASING CURVES

struct RigTransform;
struct RigBone;
struct RigSlot;
struct RigAttachment;
struct RigTranslationKeyframe;
struct RigScaleKeyframe;
struct RigRotationKeyframe;
struct RigSlotKeyframe;
struct RigEvent;
struct RigBoneAnimation;
struct RigSlotAnimation;
struct RigAnimation;

struct RigTransform {
	Vec2 translation;
	Vec2 scale;
	float degrees;
};

struct RigBone
{
	uint32_t hash;
	RigTransform rest;
};

struct RigSlot
{
	uint32_t hash;
	RigBone *bone;
	RigAttachment *rest;
	Color color;
};

struct RigAttachment
{
	uint32_t hash;
	RigSlot *slot;
	uint32_t skin;
	ImageAsset *image;
	RigTransform xform;
};

struct RigTranslationKeyframe
{
	float time;
	Vec2 translation;
};

struct RigScaleKeyframe
{
	float time;
	Vec2 scale;
};

struct RigRotationKeyframe
{
	float time;
	float degrees;
};

struct RigSlotKeyframe
{
	float time;
	Color color;
	RigAttachment *attachment;
};

struct RigEvent
{
	float time;
	uint32_t hash;
};

struct RigBoneAnimation
{
	RigBone *bone;
	uint32_t ntkeys;
	uint32_t nrkeys;
	uint32_t nskeys;
	RigTranslationKeyframe *tkeys;
	RigRotationKeyframe *rkeys;
	RigScaleKeyframe *skeys;
};

struct RigSlotAnimation
{
	RigSlot *slot;
	uint32_t nkeys;
	RigSlotKeyframe *keys;
};

struct RigAnimation
{
	uint32_t hash;
	uint32_t nbones;
	uint32_t nslots;
	uint32_t nevents;
	RigBoneAnimation *bones;
	RigSlotAnimation *slots;
	RigEvent *events;
};

struct RigAsset
{
	uint32_t nbones;
	uint32_t nslots;
	uint32_t nattachments;
	uint32_t nanims;
	RigBone *bones;
	RigSlot *slots;
	RigAttachment *attachments;
	RigAnimation *animations;
};

//------------------------------------------------------------------------------
// MAIN INTERFACE
//------------------------------------------------------------------------------

struct AssetData;

class AssetBundle {
private:
	AssetData *data;
	AssetBundle *fallback;

public:
	AssetBundle(const char* path=0, uint32_t crc=0);
	~AssetBundle();

	// Assets are keyed by name-hashes (fnv-1a)
	// inlined so that the compiler can constant-fold over string literals :)
	inline static uint32_t hash(const char* name) {
	    uint32_t hval = 0x811c9dc5;
	    while(*name) {
	        hval ^= (*name);
	        hval *= 0x01000193;
	        ++name;
	    }
	    return hval;
	}

	#ifdef DEBUG
	#define ASSET_RESULT_VERIFY(expr) auto res = (expr); if (!res) { LOG(("ASSET UNDEFINED: %s\n", name)); } return res;
	#else
	#define ASSET_RESULT_VERIFY(expr) return expr;
	#endif

	// lookup assets by name
	TextureAsset *texture(const char *name) { ASSET_RESULT_VERIFY(texture(hash(name))) }
	ImageAsset *image(const char *name) { ASSET_RESULT_VERIFY(image(hash(name))) }
	TilemapAsset *tilemap(const char *name) { ASSET_RESULT_VERIFY(tilemap(hash(name))) }
	FontAsset *font(const char *name) { ASSET_RESULT_VERIFY(font(hash(name))) }
	SampleAsset *sample(const char *name) { ASSET_RESULT_VERIFY(sample(hash(name))) }
	PaletteAsset *palette(const char *name) { ASSET_RESULT_VERIFY(palette(hash(name))) }
	
	template<typename T>
	T *userdata(const char *name) { ASSET_RESULT_VERIFY(userdata<T>(hash(name))) }

	// lookup assets by hash
	TextureAsset *texture(uint32_t hash) { return (TextureAsset*) findHeader(hash, ASSET_TYPE_TEXTURE); }
	ImageAsset *image(uint32_t hash) { return (ImageAsset*) findHeader(hash, ASSET_TYPE_IMAGE); }
	TilemapAsset *tilemap(uint32_t hash) { return (TilemapAsset*) findHeader(hash, ASSET_TYPE_TILEMAP); }
	FontAsset *font(uint32_t hash) { return (FontAsset*) findHeader(hash, ASSET_TYPE_FONT); }
	SampleAsset *sample(uint32_t hash) { return (SampleAsset*) findHeader(hash, ASSET_TYPE_SAMPLE); }
	PaletteAsset *palette(uint32_t hash) { return (PaletteAsset*) findHeader(hash, ASSET_TYPE_PALETTE); }
	
	template<typename T>
	T *userdata(uint32_t hash) { return (T*) findHeader(hash, ASSET_TYPE_USERDATA); }

	#undef ASSET_RESULT_VERIFY

	// headers are sorted by hash, so lookup is LOG(N)
	void* findHeader(uint32_t hash, uint32_t assetType);
	void setFallback(AssetBundle* fallback);

	// by default assets are initialized lazily, but this method will eagerly initialize
	// the whole asset data;
	void init();

	// release all intialized assets, but don't free the POD from memory
	void release();

};
