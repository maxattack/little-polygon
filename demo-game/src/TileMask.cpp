#include "game.h"

TileMask::TileMask(const WorldData& data) :
mWidth(data.maskWidth),
mHeight(data.maskHeight)
{
	int nbytes = (mWidth * mHeight + 7) >> 3;
	bytes = (uint8_t*) SDL_malloc(nbytes);
	memcpy(bytes, data.maskBytes, nbytes);
}

TileMask::~TileMask() {
	SDL_free(bytes);
}

bool TileMask::get(int x, int y) const {
	return x < 0 || x >= mWidth || (y >= 0 && y < mHeight && rawGet(x,y));
}

bool TileMask::rawGet(int x, int y) const {
	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(x < mWidth);
	ASSERT(y < mHeight);
	int byteIdx, localIdx; getIndices(x, y, &byteIdx, &localIdx);
	return bytes[byteIdx] & (1<<localIdx);
}

void TileMask::mark(int x, int y) {
	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(x < mWidth);
	ASSERT(y < mHeight);
	int byteIdx, localIdx; getIndices(x, y, &byteIdx, &localIdx);
	bytes[byteIdx] |= (1<<localIdx);
	
}

void TileMask::clear(int x, int y) {
	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(x < mWidth);
	ASSERT(y < mHeight);
	int byteIdx, localIdx; getIndices(x, y, &byteIdx, &localIdx);
	bytes[byteIdx] &= ~(1<<localIdx);

}

void TileMask::getIndices(int x, int y, int *byteIdx, int *localIdx) const {
	int index = x + mWidth * y;
	*byteIdx = index >> 3;
	*localIdx = index - ((*byteIdx) << 3);
}

bool TileMask::check(lpVec topLeft, lpVec bottomRight) const {
	int left = floorToInt(topLeft.x);
	int right = floorToInt(bottomRight.x);
	int bottom = floorToInt(bottomRight.y);
	int top = floorToInt(topLeft.y);
	for(int y=top; y<=bottom; ++y)
	for(int x=left; x<=right; ++x) {
		if (get(x,y)) { return true; }
	}
	return false;
}

bool TileMask::checkLeft(lpVec topLeft, lpVec bottomRight, float *outResult) const {
	int left = floorToInt(topLeft.x);
	int right = floorToInt(bottomRight.x);
	int bottom = floorToInt(bottomRight.y);
	int top = floorToInt(topLeft.y);
	for(int x=left; x<=right; ++x)
	for(int y=top; y<=bottom; ++y) {
		if (get(x,y)) {
			*outResult = x + 1.0f - topLeft.x + kSlop;
			return true;
		}
	}
	*outResult = 0.0f;
	return false;
}

bool TileMask::checkRight(lpVec topLeft, lpVec bottomRight, float *outResult) const {
	int left = floorToInt(topLeft.x);
	int right = floorToInt(bottomRight.x);
	int bottom = floorToInt(bottomRight.y);
	int top = floorToInt(topLeft.y);
	for(int x=right; x>=left; --x)
	for(int y=top; y<=bottom; ++y) {
		if (get(x,y)) {
			*outResult = x - bottomRight.x - kSlop;
			return true;
		}
	}
	*outResult = 0.0f;
	return false;

}

bool TileMask::checkTop(lpVec topLeft, lpVec bottomRight, float *outResult) const {
	int left = floorToInt(topLeft.x);
	int right = floorToInt(bottomRight.x);
	int bottom = floorToInt(bottomRight.y);
	int top = floorToInt(topLeft.y);
	for(int y=bottom; y>=top; --y)
	for(int x=left; x<=right; ++x) {
		if (get(x,y)) {
			*outResult = y + 1.0f - topLeft.y + kSlop;
			return true;
		}
	}
	*outResult = 0.0f;
	return false;

}

bool TileMask::checkBottom(lpVec topLeft, lpVec bottomRight, float *outResult) const {
	int left = floorToInt(topLeft.x);
	int right = floorToInt(bottomRight.x);
	int bottom = floorToInt(bottomRight.y);
	int top = floorToInt(topLeft.y);
	for(int y=top; y<=bottom; ++y)
	for(int x=left; x<=right; ++x) {
		if (get(x,y)) {
			*outResult = y - bottomRight.y - kSlop;
			return true;
		}
	}
	*outResult = 0.0f;
	return false;
}

void TileMask::debugDraw() {
	for(int y=0; y<mHeight; ++y)
	for(int x=0; x<mWidth; ++x) {
		if (rawGet(x,y)) {
			lpLines.plotBox(vec(x,y), vec(x+1, y+1), rgb(333333));
		}
	}
		
}

bool TileMask::isFloor(int x, int y) const {
	return !get(x,y) && get(x, y+1);
}


