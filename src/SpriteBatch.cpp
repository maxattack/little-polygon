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

#include "littlepolygon/sprites.h"
#include "littlepolygon/bitset.h"



SpriteBatch::SpriteBatch(int nlayers) : mLayerCount(nlayers) {
	mLayers = (Layer*) LITTLE_POLYGON_MALLOC(mLayerCount * sizeof(Layer));
	for(int i=0; i<mLayerCount; ++i) {
		new (mLayers+i) Layer();
	}
}

SpriteBatch::~SpriteBatch() {
	for(int i=0; i<mLayerCount; ++i) {
		mLayers[i].~Layer();
	}
	LITTLE_POLYGON_FREE(mLayers);
}

Sprite *SpriteBatch::addSprite(
	int layer,
	ImageAsset *image,
	const AffineMatrix& xform,
	int frame,
	Color c,
	float depth
) {
	ASSERT(layer >= 0);
	ASSERT(layer < mLayerCount);
	
	auto result = mSprites.alloc(this, layer, mLayers[layer].count());
	
	auto drawCall = mLayers[layer].alloc();
	drawCall->sprite = result;
	drawCall->img = image;
	drawCall->xform = xform;
	drawCall->frame = frame;
	drawCall->color = c;
	drawCall->depth = depth;
	
	return result;
}

void SpriteBatch::draw(SpritePlotter *plotter) {
	for(int i=0; i<mLayerCount; ++i) {
		for(auto& draw : mLayers[i]) {
			plotter->drawImage(draw.img, draw.xform, draw.frame, draw.color, draw.depth);
		}
	}
}

void SpriteBatch::clear() {
	for(auto s=mSprites.list(); s.next();) {
		s->release();
	}
}

Sprite::Sprite(SpriteBatch *batch, int layer, int index) :
mBatch(batch), mLayer(layer), mIndex(index) {
}

void Sprite::clear() {
	mBatch = 0;
	mLayer = 0;
	mIndex = 0;
}

const AffineMatrix& Sprite::transform() const {
	return mBatch->mLayers[mLayer][mIndex].xform;
}

void Sprite::setTransform(const AffineMatrix& xform) {
	mBatch->mLayers[mLayer][mIndex].xform = xform;
}

ImageAsset* Sprite::image() const {
	return mBatch->mLayers[mLayer][mIndex].img;
}

void Sprite::setImage(ImageAsset *image) {
	mBatch->mLayers[mLayer][mIndex].img = image;
}

void Sprite::setLayer(int i) {
	ASSERT(i >= 0 && i < mBatch->mLayerCount);
	if (i != mLayer) {

		// remove from the current layer, saving current call
		auto& layer = mBatch->mLayers[mLayer];
		auto drawCall = layer[mIndex];
		layer.release(&layer[mIndex]);
		
		// if a record was relocated into this slot, then updates it's
		// sprite handle's index
		if (mIndex < layer.count()) {
			layer[mIndex].sprite->mIndex = mIndex;
		}
		
		// add to new layer
		mLayer = i;
		mIndex = mBatch->mLayers[i].count();
		mBatch->mLayers[i].alloc();
		mBatch->mLayers[i][mIndex] = drawCall;
	}
	
}

Color Sprite::fill() const {
	return mBatch->mLayers[mLayer][mIndex].color;
}

void Sprite::setFill(Color aFill) {
	mBatch->mLayers[mLayer][mIndex].color = aFill;
}

int Sprite::frame() const {
	return mBatch->mLayers[mLayer][mIndex].frame;
}

void Sprite::setFrame(int frame) {
	ASSERT(frame < image()->nframes);
	mBatch->mLayers[mLayer][mIndex].frame = frame;
}


void Sprite::release() {
	// remove from the current layer
	auto& layer = mBatch->mLayers[mLayer];
	layer.release(&layer[mIndex]);
	
	// if a record was relocated into this slot, then updates it's
	// sprite handle's index
	if (mIndex < layer.count()) {
		layer[mIndex].sprite->mIndex = mIndex;
	}
	
	mIndex = 0;
	mLayer = 0;
	auto b = mBatch;
	mBatch = 0;
	
	// release thy self
	b->mSprites.release(this);
	

}

