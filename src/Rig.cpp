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

data(asset),
localAttitudes(  (Attitude*)  SDL_calloc(asset->nbones, sizeof(Attitude))  ),
localTransforms( (AffineMatrix*)  SDL_calloc(asset->nbones, sizeof(AffineMatrix))  ),
parents(         (AffineMatrix**) SDL_calloc(asset->nbones, sizeof(AffineMatrix*)) ),
worldTransforms( (AffineMatrix*)  SDL_calloc(asset->nbones, sizeof(AffineMatrix))  ),

timelineMask(asset->ntimeslines),
currentKeyframes( (int*) SDL_calloc(asset->ntimeslines, sizeof(int)) ),

currentLayer(asset->defaultLayer),
currentAnimation(0)

{
	// INITIALIZE REST POSE
	for(int i=0; i<data->nbones; ++i) {
		auto& bone = data->bones[i];
		localAttitudes[i].radians = bone.defaultTransform.radians;
		localAttitudes[i].scale = bone.defaultTransform.scale;
		localTransforms[i] = bone.defaultTransform.concatenatedMatrix();
		parents[i] = worldTransforms + bone.parentIndex;
	}
	
	// just set dirty bit?
	setRootTransform(matIdentity());
}

Rig::~Rig()
{
	SDL_free(localTransforms);
	SDL_free(parents);
	SDL_free(worldTransforms);
	SDL_free(currentKeyframes);
}

void Rig::setRootTransform(const AffineMatrix& mat)
{
	localTransforms[0] = mat;
	worldTransforms[0] = mat;
	computeWorldTransforms();
}

const AffineMatrix* Rig::findTransform(uint32_t hash) const
{
	for(int i=0; i<data->nbones; ++i) {
		if (data->bones[i].hash == hash) {
			return worldTransforms + i;
		}
	}
	return 0;
}

void Rig::draw(SpritePlotter* plotter)
{
	for(int i=0; i<data->nattachments; ++i)
	{
		auto& attach = data->attachments[i];
		if (attach.layerHash == 0 || attach.layerHash == currentLayer) {
			plotter->drawImage(
				attach.image,
				worldTransforms[attach.slot->boneIndex] * attach.xform
			);
		}
	}
}

void Rig::computeWorldTransforms()
{
	// skip root
	for(int i=1; i<data->nbones; ++i) {
		worldTransforms[i] = (*parents[i]) * localTransforms[i];
	}
}

void Rig::setAnimation(uint32_t hash)
{
	// VERIFY THAT HASH IS VALID
	if (hash == currentAnimation) {
		return;
	} else {
		for(int i=0; i<data->nanims; ++i) {
			if (hash == data->anims[i].hash) {
				goto Validated;
			}
		}
		return;
	}
Validated:
	
	// RESET TIMER, UPDATE TIMELINE MASK
	currentAnimation = hash;
	currentTime = 0.0f;
	timelineMask.clear();
	for(int i=0; i<data->ntimeslines; ++i) {
		if (data->timelines[i].animHash == hash) {
			timelineMask.mark(i);
			currentKeyframes[i] = 0;
		}
	}
	
	// APPLY FIRST FRAME
	// TODO
}

void Rig::tick(float dt)
{
	// TODO
}

void Rig::resetPose()
{
	// TODO
}



