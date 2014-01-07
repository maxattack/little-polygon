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

void initialize(TextureAsset *asset) {
	if(asset->textureHandle == 0) {
		glGenTextures(1, &asset->textureHandle);
		glBindTexture(GL_TEXTURE_2D, asset->textureHandle);
		if (asset->flags & TEXTURE_FLAG_FILTER) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		if (asset->flags & TEXTURE_FLAG_REPEAT) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		uLongf size = 4 * asset->w * asset->h;
		Bytef *scratch = (Bytef *) LITTLE_POLYGON_MALLOC(size);
		int result = uncompress(scratch, &size, (const Bytef*)asset->compressedData, asset->compressedSize);
		CHECK(result == Z_OK);
		int fmt = asset->format();
		glTexImage2D(GL_TEXTURE_2D, 0, fmt, asset->w, asset->h, 0, fmt, GL_UNSIGNED_BYTE, scratch);
		LITTLE_POLYGON_FREE(scratch);
	}
}

void release(TextureAsset *asset) {
	if (asset->textureHandle) {
		glDeleteTextures(1, &asset->textureHandle);
		asset->textureHandle = 0;
	}
}

void bind(TextureAsset *asset) {
	initialize(asset);
	glBindTexture(GL_TEXTURE_2D, asset->textureHandle);
}

