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

AssetBundle* createAssetBundle(const char* path, uint32_t crc) {
	SDL_RWops* file = SDL_RWFromFile(path, "rb");
	
	// read length and count
	int length = SDL_ReadLE32(file);
	int count = SDL_ReadLE32(file);

	// read data
	AssetBundle *bundle = (AssetBundle*) LITTLE_POLYGON_MALLOC(sizeof(AssetBundle) + length);
	void *result = (bundle+1);
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
	bundle->headers = reinterpret_cast<AssetBundle::Header*>(result);
	bundle->fallback = 0;
	return bundle;
}

void destroy(AssetBundle *bundle) {
	release(bundle);
	LITTLE_POLYGON_FREE(bundle);
}

void* AssetBundle::findHeader(uint32_t hash, uint32_t assetType) {
	// headers are sorted on their hash, so we can binary search
	int imin = 0;
	int imax = assetCount-1;
	while (imax >= imin) {
		int i = (imin + imax) >> 1;
		if (headers[i].hash == hash) {
			if (headers[i].type == assetType) {
				return headers[i].data;
			} else {
				break;
			}
		} else if (headers[i].hash < hash) {
			imin = i+1;
		} else {
			imax = i-1;
		}
	}
	return fallback ? fallback->findHeader(hash, assetType) : 0;
}

void initialize(AssetBundle* bundle) {
	if (bundle->assetCount) { 
		for(int i=0; i<bundle->assetCount; ++i) {
			switch(bundle->headers[i].type) {
				case ASSET_TYPE_TEXTURE:
					initialize( (TextureAsset*)bundle->headers[i].data );
					break;
				case ASSET_TYPE_FONT:
					initialize( &(((FontAsset*)bundle->headers[i].data)->texture) );
					break;
				case ASSET_TYPE_SAMPLE:
					initialize( (SampleAsset*)bundle->headers[i].data );
					break;
				case ASSET_TYPE_TILEMAP:
					initialize( (TilemapAsset*)bundle->headers[i].data );
					break;
				default:
					break;
			}
		}
	}
}

void release(AssetBundle* bundle) {
	if (bundle->assetCount) { 
		for(int i=0; i<bundle->assetCount; ++i) {
			switch(bundle->headers[i].type) {
				case ASSET_TYPE_TEXTURE:
					release( (TextureAsset*)bundle->headers[i].data );
					break;
				case ASSET_TYPE_FONT:
					release( &(((FontAsset*)bundle->headers[i].data)->texture) );
					break;
				case ASSET_TYPE_SAMPLE:
					release( (SampleAsset*)bundle->headers[i].data );
					break;
				case ASSET_TYPE_TILEMAP:
					release( (TilemapAsset*)bundle->headers[i].data );
					break;
				default:
					break;				
			}
		}
	}
}
