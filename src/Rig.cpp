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

#include "littlepolygon/rig.h"

Rig::Rig(const RigAsset* asset) :

mAsset(asset),
mLocalTransforms( (AffineMatrix*) SDL_calloc(asset->nbones, sizeof(AffineMatrix)) ),
mParents( (AffineMatrix**) SDL_calloc(asset->nbones, sizeof(AffineMatrix*)) ),
mWorldTransforms( (AffineMatrix*) SDL_calloc(asset->nbones, sizeof(AffineMatrix)) ),
mAttachTransforms( (AffineMatrix*) SDL_calloc(asset->nattachments, sizeof(AffineMatrix)) ),

currentLayer(asset->defaultLayer)

{
	// INITIALIZE REST POSE
	for(int i=0; i<mAsset->nbones; ++i) {
		auto& bone = mAsset->bones[i];
		mLocalTransforms[i] = bone.defaultTransform.concatenatedMatrix();
		if (i == 0) {
			mParents[i] = 0;
		} else {
			ASSERT(bone.parent != 0);
			mParents[i] = mWorldTransforms + getIndex(bone.parent);
		}
	}
	
	for(int i=0; i<mAsset->nattachments; ++i) {
		mAttachTransforms[i] = mAsset->attachments[i].xform.concatenatedMatrix();
	}
	
	
	setRootTransform(matIdentity());
}

Rig::~Rig() {
	SDL_free(mLocalTransforms);
	SDL_free(mParents);
	SDL_free(mWorldTransforms);
}

void Rig::setRootTransform(const AffineMatrix& mat)
{
	mLocalTransforms[0] = mat;
	mWorldTransforms[0] = mat;
	computeWorldTransforms();
}

const AffineMatrix* Rig::findTransform(uint32_t hash) const
{
	for(int i=0; i<mAsset->nbones; ++i) {
		if (mAsset->bones[i].hash == hash) {
			return mWorldTransforms + i;
		}
	}
	return 0;
}

void Rig::draw(SpritePlotter* plotter)
{
	// TODO: SLOT FLIPBOOKING
	for(int i=0; i<mAsset->nattachments; ++i)
	{
		auto& attach = mAsset->attachments[i];
		if (attach.layerHash == 0 || attach.layerHash == currentLayer) {
			plotter->drawImage(
				attach.image,
				mWorldTransforms[getIndex(attach.slot->bone)] * mAttachTransforms[i]
			);
		}
	}
}

void Rig::computeWorldTransforms()
{
	for(int i=1; i<mAsset->nbones; ++i) {
		mWorldTransforms[i] = (*mParents[i]) * mLocalTransforms[i];
	}
}

