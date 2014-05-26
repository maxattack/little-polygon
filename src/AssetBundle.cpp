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

#include "littlepolygon/assets.h"

struct AssetHeader {
	uint32_t hash, type;
	void* data;
};

struct AssetData {
	size_t assetCount;

	// if we fail to find an asset, optionally check a fallback
	// (for "pusinging" level content on top of shared content)
	AssetHeader headers[1];
};

AssetBundle::AssetBundle(const char* path, uint32_t crc) : data(0), fallback(0) {
	if (path == 0 || strlen(path) == 0) {
		return;
	}

	SDL_RWops* file = SDL_RWFromFile(path, "rb");
	
	// read length and count
	int pointerWidth = SDL_ReadLE32(file);
	if (pointerWidth != 8 * sizeof(void*)) {
		LOG(("Asset Wordsize is wrong (%d)\n", pointerWidth));
		SDL_RWclose(file);
		return;
	}

	int length = SDL_ReadLE32(file);
	int count = SDL_ReadLE32(file);

	// read data
	data = (AssetData*) SDL_malloc(sizeof(AssetData)-sizeof(AssetHeader) + length);
	void *result = &(data->headers);
	if (SDL_RWread(file, result, length, 1) == -1) {
		SDL_free(data);
		SDL_RWclose(file);
		data = 0;
		return;
	}

	// read pointer fixup
	uint8_t *bytes = (uint8_t*) result;
	uint32_t offset;
	while(SDL_RWread(file, &offset, sizeof(uint32_t), 1)) {
		*((uintptr_t*)(bytes + offset)) += uintptr_t(bytes);
	}
	SDL_RWclose(file);

	data->assetCount = count;
}

AssetBundle::~AssetBundle() {
	if (data) {
		release();
		SDL_free(data);
	}
}

void* AssetBundle::findHeader(uint32_t hash, uint32_t assetType) {
	if (data) {
		// headers are sorted on their hash, so we can binary search
		int imin = 0;
		int imax = data->assetCount-1;
		while (imax >= imin) {
			int i = (imin + imax) >> 1;
			if (data->headers[i].hash == hash) {
				if (data->headers[i].type == assetType) {
					return data->headers[i].data;
				} else {
					break;
				}
			} else if (data->headers[i].hash < hash) {
				imin = i+1;
			} else {
				imax = i-1;
			}
		}
	}
	return fallback ? fallback->findHeader(hash, assetType) : 0;
}

void AssetBundle::setFallback(AssetBundle *aFallback) {
	fallback = aFallback;
}

void AssetBundle::init() {
	if (data && data->assetCount) { 
		for(int i=0; i<data->assetCount; ++i) {
			switch(data->headers[i].type) {
				case ASSET_TYPE_TEXTURE:
					((TextureAsset*)data->headers[i].data)->init();
					break;
				case ASSET_TYPE_FONT:
					(((FontAsset*)data->headers[i].data)->texture).init();
					break;
				case ASSET_TYPE_SAMPLE:
					((SampleAsset*)data->headers[i].data)->init();
					break;
				case ASSET_TYPE_TILEMAP:
					((TilemapAsset*)data->headers[i].data)->init();
					break;
				default:
					break;
			}
		}
	}
}

void AssetBundle::release() {
	if (data && data->assetCount) { 
		for(int i=0; i<data->assetCount; ++i) {
			switch(data->headers[i].type) {
				case ASSET_TYPE_TEXTURE:
					((TextureAsset*)data->headers[i].data)->release();
					break;
				case ASSET_TYPE_FONT:
					(((FontAsset*)data->headers[i].data)->texture).release();
					break;
				case ASSET_TYPE_SAMPLE:
					((SampleAsset*)data->headers[i].data)->release();
					break;
				case ASSET_TYPE_TILEMAP:
					((TilemapAsset*)data->headers[i].data)->release();
					break;
				default:
					break;				
			}
		}
	}
}
