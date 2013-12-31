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

#include "littlepolygon.h"

int AssetBundle::load(const char *path, uint32_t crc) {
	// check if we need to unload something first
	unload();

	SDL_RWops* file = SDL_RWFromFile(path, "rb");
	
	// read length and count
	int length = SDL_ReadLE32(file);
	int count = SDL_ReadLE32(file);

	// read data
	void *result = LPMALLOC(length);
	if (SDL_RWread(file, result, length, 1) == -1) {
		LPFREE(result);
		return 1;
	}

	// read pointer fixup
	uint8_t *bytes = (uint8_t*) result;
	uint32_t offset;
	while(SDL_RWread(file, &offset, sizeof(uint32_t), 1)) {
		*reinterpret_cast<size_t*>(&bytes[offset]) += size_t(bytes);
	}
	SDL_RWclose(file);

	assetCount = count;
	headers = reinterpret_cast<AssetBundle::Header*>(result);
	return 0;
}

void AssetBundle::unload() {
	release();
	if (assetCount > 0) {
		LPFREE(headers);
		assetCount = 0;
		headers = 0;
	}
}

void* AssetBundle::findHeader(uint32_t hash, uint32_t assetType) {
	// headers are sorted on their hash, so we can binary search it
	int imin = 0;
	int imax = assetCount-1;
	while (imax >= imin) {
		int i = (imin + imax) >> 1;
		if (headers[i].hash == hash) {
			return headers[i].type == assetType ? headers[i].data : 0;
		} else if (headers[i].hash < hash) {
			imin = i+1;
		} else {
			imax = i-1;
		}
	}
	return 0;
}

void AssetBundle::init() {
	if (headers) { 
		for(int i=0; i<assetCount; ++i) {
			switch(headers[i].type) {
				case ASSET_TYPE_TEXTURE:
					((TextureAsset*)headers[i].data)->init();
					break;
				case ASSET_TYPE_FONT:
					((FontAsset*)headers[i].data)->texture.init();
					break;
				case ASSET_TYPE_SAMPLE:
					((SampleAsset*)headers[i].data)->init();
					break;
				case ASSET_TYPE_TILEMAP:
					((TilemapAsset*)headers[i].data)->tileAtlas.init();
					break;
				default:
					break;
			}
		}
	}
}

void AssetBundle::release() {
	if (headers) { 
		for(int i=0; i<assetCount; ++i) {
			switch(headers[i].type) {
				case ASSET_TYPE_TEXTURE:
					((TextureAsset*)headers[i].data)->release();
					break;
				case ASSET_TYPE_FONT:
					((FontAsset*)headers[i].data)->texture.release();
					break;
				case ASSET_TYPE_SAMPLE:
					((SampleAsset*)headers[i].data)->release();
					break;
				case ASSET_TYPE_TILEMAP:
					((TilemapAsset*)headers[i].data)->tileAtlas.release();
					break;
				default:
					break;				
			}
		}
	}
}
