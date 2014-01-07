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
#include <zlib.h>

void initialize(TilemapAsset *map) {
	initialize(&map->tileAtlas);
	if (!map->data) {
		uLongf size = sizeof(uint8_pair_t) * map->mw * map->mh;
		map->data = (uint8_pair_t *) LITTLE_POLYGON_MALLOC( size );
		int result = uncompress((Bytef*)map->data, &size, (const Bytef*)map->compressedData, map->compressedSize);
		CHECK(result == Z_OK);
	}

}

void release(TilemapAsset *map) {
	release(&map->tileAtlas);
	if (map->data) {
		LITTLE_POLYGON_FREE(map->data);
		map->data = 0;
	}
}