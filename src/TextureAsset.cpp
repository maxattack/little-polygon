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

void TextureAsset::init()
{
	if(handle == 0) {
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_2D, handle);
		if (flags & TEXTURE_FLAG_FILTER) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		if (flags & TEXTURE_FLAG_REPEAT) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		uLongf size = 4 * w * h;
		Bytef *scratch = (Bytef *) calloc(w*h, 4);
		#if DEBUG
		int result =
		#endif
		uncompress(scratch, &size, (const Bytef*)compressedData, compressedSize);
		ASSERT(result == Z_OK);
		int fmt = format();
		glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, scratch);
		free(scratch);
	}
}

void TextureAsset::release()
{
	if (handle) {
		glDeleteTextures(1, &handle);
		handle = 0;
	}
}

void TextureAsset::bind()
{
	init();
	glBindTexture(GL_TEXTURE_2D, handle);
}

