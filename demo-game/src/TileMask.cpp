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

bool TileMask::check(vec2 topLeft, vec2 bottomRight) const {
	int left = topLeft.x;
	int right = bottomRight.x;
	int bottom = bottomRight.y;
	int top = topLeft.y;
	for(int y=top; y<=bottom; ++y)
	for(int x=left; x<=right; ++x) {
		if (get(x,y)) { return true; }
	}
	return false;
}

bool TileMask::checkLeft(vec2 bottomLeft, vec2 topRight, float *outResult) const {
	int left = bottomLeft.x;
	int right = topRight.x;
	int bottom = bottomLeft.y;
	int top = topRight.y;
	for(int x=left; x<=right; ++x)
	for(int y=top; y<=bottom; ++y) {
		if (get(x,y)) {
			*outResult = x + 1.0f - bottomLeft.x + kSlop;
			return true;
		}
	}
	*outResult = 0.0f;
	return false;
}

bool TileMask::checkRight(vec2 bottomLeft, vec2 topRight, float *outResult) const {
	int left = bottomLeft.x;
	int right = topRight.x;
	int bottom = bottomLeft.y;
	int top = topRight.y;
	for(int x=right; x>=left; --x)
	for(int y=top; y<=bottom; ++y) {
		if (get(x,y)) {
			*outResult = x - topRight.x - kSlop;
			return true;
		}
	}
	*outResult = 0.0f;
	return false;

}

bool TileMask::checkTop(vec2 bottomLeft, vec2 topRight, float *outResult) const {
	int left = bottomLeft.x;
	int right = topRight.x;
	int bottom = bottomLeft.y;
	int top = topRight.y;
	for(int y=bottom; y>=top; --y)
	for(int x=left; x<=right; ++x) {
		if (get(x,y)) {
			*outResult = y + 1.0f - topRight.y + kSlop;
			return true;
		}
	}
	*outResult = 0.0f;
	return false;

}

bool TileMask::checkBottom(vec2 bottomLeft, vec2 topRight, float *outResult) const {
	int left = bottomLeft.x;
	int right = topRight.x;
	int bottom = bottomLeft.y;
	int top = topRight.y;
	for(int y=top; y<=bottom; ++y)
	for(int x=left; x<=right; ++x) {
		if (get(x,y)) {
			*outResult = y - bottomLeft.y - kSlop;
			return true;
		}
	}
	*outResult = 0.0f;
	return false;
}

void TileMask::debugDraw() {
	for(int y=0; y<mHeight; ++y)
	for(int x=0; x<mWidth; ++x) {
		if (get(x,y)) {
			gLines.plotBox(vec(x,y), vec(x+1, y+1), rgb(333333));
		}
	}
		
}


