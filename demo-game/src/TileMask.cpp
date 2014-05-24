#include "game.h"

TileMask::TileMask(int w, int h) :
mWidth(w),
mHeight(h)
{
	int nbytes = (w * h + 7) >> 3;
	bytes = (uint8_t*) malloc(nbytes);
}

TileMask::TileMask(const Data* data) :
mWidth(data->w),
mHeight(data->h)
{
	int nbytes = (mWidth * mHeight + 7) >> 3;
	bytes = (uint8_t*) malloc(nbytes);
	memcpy(bytes, data->bytes, nbytes);
}

TileMask::~TileMask() {
	free(bytes);
}

bool TileMask::get(int x, int y) const {
	int byteIdx, localIdx; getIndices(x, y, &byteIdx, &localIdx);
	return bytes[byteIdx] & (1<<localIdx);
}

void TileMask::mark(int x, int y) {
	int byteIdx, localIdx; getIndices(x, y, &byteIdx, &localIdx);
	bytes[byteIdx] |= (1<<localIdx);
	
}

void TileMask::clear(int x, int y) {
	int byteIdx, localIdx; getIndices(x, y, &byteIdx, &localIdx);
	bytes[byteIdx] &= ~(1<<localIdx);

}

void TileMask::getIndices(int x, int y, int *byteIdx, int *localIdx) const {
	int index = x + mWidth * y;
	*byteIdx = index >> 3;
	*localIdx = index - ((*byteIdx) << 3);
}
