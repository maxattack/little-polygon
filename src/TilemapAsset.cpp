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
#include <zlib.h>

void TilemapAsset::init() {
	tileAtlas.init();
	if (!data) {
		uLongf size = sizeof(uint8_pair_t) * mw * mh;
		data = (uint8_pair_t *) malloc( size );
		int result = uncompress((Bytef*)data, &size, (const Bytef*)compressedData, compressedSize);
		assert(result == Z_OK);
	}

}

void TilemapAsset::release() {
	tileAtlas.release();
	free(data);
	data = 0;
}

void TilemapAsset::reload() {
	free(data);
	data = 0;
	init();
}
