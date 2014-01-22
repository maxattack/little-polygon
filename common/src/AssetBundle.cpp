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

#include "littlepolygon_assets.h"

struct Header {
	uint32_t hash, type;
	void* data;
};

struct AssetBundle {
	size_t assetCount;

	// if we fail to find an asset, optionally check a fallback
	// (for "pusinging" level content on top of shared content)
	AssetBundle *fallback;
	Header headers[1];
};

AssetRef loadAssets(const char* path, uint32_t crc) {
	SDL_RWops* file = SDL_RWFromFile(path, "rb");
	
	// read length and count
	int length = SDL_ReadLE32(file);
	int count = SDL_ReadLE32(file);

	// read data
	AssetBundle *bundle = (AssetBundle*) LITTLE_POLYGON_MALLOC(sizeof(AssetBundle)-sizeof(Header) + length);
	void *result = &(bundle->headers);
	if (SDL_RWread(file, result, length, 1) == -1) {
		LITTLE_POLYGON_FREE(bundle);
		return 0;
	}

	// read pointer fixup
	uint8_t *bytes = (uint8_t*) result;
	uint32_t offset;
	while(SDL_RWread(file, &offset, sizeof(uint32_t), 1)) {
		*((ptrdiff_t*)(bytes + offset)) += ptrdiff_t(bytes);
	}
	SDL_RWclose(file);

	bundle->assetCount = count;
	bundle->fallback = 0;
	return bundle;
}

void AssetRef::destroy() {
	release();
	LITTLE_POLYGON_FREE(bundle);
}

void* AssetRef::findHeader(uint32_t hash, uint32_t assetType) {
	// headers are sorted on their hash, so we can binary search
	int imin = 0;
	int imax = bundle->assetCount-1;
	while (imax >= imin) {
		int i = (imin + imax) >> 1;
		if (bundle->headers[i].hash == hash) {
			if (bundle->headers[i].type == assetType) {
				return bundle->headers[i].data;
			} else {
				break;
			}
		} else if (bundle->headers[i].hash < hash) {
			imin = i+1;
		} else {
			imax = i-1;
		}
	}
	return bundle->fallback ? AssetRef(bundle->fallback).findHeader(hash, assetType) : 0;
}

void AssetRef::init() {
	if (bundle->assetCount) { 
		for(int i=0; i<bundle->assetCount; ++i) {
			switch(bundle->headers[i].type) {
				case ASSET_TYPE_TEXTURE:
					((TextureAsset*)bundle->headers[i].data)->init();
					break;
				case ASSET_TYPE_FONT:
					(((FontAsset*)bundle->headers[i].data)->texture).init();
					break;
				case ASSET_TYPE_SAMPLE:
					((SampleAsset*)bundle->headers[i].data)->init();
					break;
				case ASSET_TYPE_TILEMAP:
					((TilemapAsset*)bundle->headers[i].data)->init();
					break;
				default:
					break;
			}
		}
	}
}

void AssetRef::release() {
	if (bundle->assetCount) { 
		for(int i=0; i<bundle->assetCount; ++i) {
			switch(bundle->headers[i].type) {
				case ASSET_TYPE_TEXTURE:
					((TextureAsset*)bundle->headers[i].data)->release();
					break;
				case ASSET_TYPE_FONT:
					(((FontAsset*)bundle->headers[i].data)->texture).release();
					break;
				case ASSET_TYPE_SAMPLE:
					((SampleAsset*)bundle->headers[i].data)->release();
					break;
				case ASSET_TYPE_TILEMAP:
					((TilemapAsset*)bundle->headers[i].data)->release();
					break;
				default:
					break;				
			}
		}
	}
}
