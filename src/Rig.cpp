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
localAttitudes((Attitude*)      SDL_calloc(data->nbones, sizeof(Attitude))),
localTransforms((AffineMatrix*) SDL_calloc(data->nbones, sizeof(AffineMatrix))),
worldTransforms((AffineMatrix*) SDL_calloc(data->nbones, sizeof(AffineMatrix))),

timelineMask(data->ntimeslines),
currentKeyframes((unsigned*)    SDL_calloc(data->ntimeslines, sizeof(unsigned))),

currentLayer(data->defaultLayer),
currentAnimation(0),

xformDirty(true)

{
	setDefaultPose();
	setRootTransform(matIdentity());
}

Rig::~Rig()
{
	SDL_free(localAttitudes);
	SDL_free(localTransforms);
	SDL_free(worldTransforms);
	SDL_free(currentKeyframes);
}

void Rig::setRootTransform(const AffineMatrix& mat)
{
	localTransforms[0] = mat;
	worldTransforms[0] = mat;
	xformDirty = true;
}

const AffineMatrix* Rig::findTransform(const char *name) const
{
	uint32_t hash = fnv1a(name);
	for(unsigned i=0; i<data->nbones; ++i) {
		if (data->bones[i].hash == hash) {
			return worldTransforms + i;
		}
	}
	LOG(("Bone Undefined: %s\n", name));
	return 0;
}


void Rig::refreshTransforms()
{
	if (xformDirty) {
		computeWorldTransforms();
	}
}

void Rig::draw(SpritePlotter* plotter)
{
	// OPTIMIZATION CANDIDATES:
	// - maintain a layer-mask over attachments?

	refreshTransforms();
	for(unsigned i=0; i<data->nattachments; ++i)
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

void Rig::setLayer(const char *layerName) {
	uint32_t hash = fnv1a(layerName);
	currentLayer = hash;
}

void Rig::setAnimation(const char *animName)
{
	uint32_t hash = fnv1a(animName);
	
	// VALIDATE
	if (currentAnimation && hash == currentAnimation->hash) {
		return;
	} else {
		bool found = false;
		for(unsigned i=0; i<data->nanims; ++i) {
			if (hash == data->anims[i].hash) {
				currentAnimation = data->anims + i;
				found = true;
				break;
			}
		}
		if (!found) {
			LOG(("Animation Undefined: %s\n", animName));
			return;
		}
	}
	
	// RESET TIMER, UPDATE TIMELINE MASK
	currentTime = 0.0f;
	timelineMask.clear();
	for(unsigned i=0; i<data->ntimeslines; ++i) {
		if (data->timelines[i].animHash == hash) {
			timelineMask.mark(i);
			currentKeyframes[i] = 0;
		}
	}
	
	// APPLY FIRST FRAME
	setDefaultPose();
	resetTime();
}


void Rig::resetPose()
{
	currentAnimation = 0;
	timelineMask.clear();
	setDefaultPose();
	computeWorldTransforms();
}

void Rig::resetTime()
{
	currentTime = 0.0f;
	if (currentAnimation) {
		for(BitLister i(&timelineMask); i.next(); ) {
			currentKeyframes[i.index()] = 0;
			applyTimeline(i.index());
		}
		xformDirty = true;
	}
}

void Rig::tick(float dt)
{
	if (currentAnimation) {
		// UPDATE TIME
		// (just wrapping for now)
		currentTime = fmodf(currentTime + dt, currentAnimation->duration);
		if (currentTime < 0.0f) { currentTime += currentAnimation->duration; }
		
		// UPDATE TIMELINES
		for(BitLister i(&timelineMask); i.next(); ) {
			updateTimeline(i.index());
			applyTimeline(i.index());
		}
		
		xformDirty = true;
	}
}

void Rig::updateTimeline(int i)
{
	auto& tl = data->timelines[i];
	
	// UPDATE KEYFRAME
	auto& kf = currentKeyframes[i];
	if (tl.times[kf] < currentTime) {
		// SEARCH FORWARD
		while (kf < tl.nkeyframes-1 && tl.times[kf+1] < currentTime) {
			++kf;
		}
	} else {
		// SEARCH BACKWARD
		while(kf > 0 && tl.times[kf] > currentTime) {
			--kf;
		}
	}
}

void Rig::applyTimeline(int i)
{
	auto& tl = data->timelines[i];
	auto& kf = currentKeyframes[i];
	auto bi = tl.boneIndex;
	
	// OPTIMIZATION CANDIDATES:
	// - separate loop for different kinds of timelines?
	// - maintain a dirty-mask over attitudes?
	
	if (kf == tl.nkeyframes-1) {

		// APPLY KEYFRAME DIRECTLY
		switch(tl.kind) {
			case kTimelineTranslation:
				localTransforms[bi].t = tl.translationValues[kf];
				break;
			case kTimelineRotation:
				localAttitudes[bi].radians = tl.rotationValues[kf];
				localAttitudes[bi].applyTo(localTransforms[bi]);
				break;
			case kTimelineScale:
				localAttitudes[bi].scale = tl.scaleValues[kf];
				localAttitudes[bi].applyTo(localTransforms[bi]);
				break;
			default:
				break;
		}
		
	} else {

		// TWEEN KEYFRAME
		auto tween = (currentTime - tl.times[kf]) / (tl.times[kf+1] - tl.times[kf]);
		
		switch(tl.kind) {
			case kTimelineTranslation:
				localTransforms[bi].t = lerp(tl.translationValues[kf], tl.translationValues[kf+1], tween);
				break;
			case kTimelineRotation:
				localAttitudes[bi].radians = lerpRadians(tl.rotationValues[kf], tl.rotationValues[kf+1], tween);
				localAttitudes[bi].applyTo(localTransforms[bi]);
				break;
			case kTimelineScale:
				localAttitudes[bi].scale = lerp(tl.scaleValues[kf], tl.scaleValues[kf+1], tween);
				localAttitudes[bi].applyTo(localTransforms[bi]);
				break;
			default:
				break;
		}
		
	}
}

void Rig::setDefaultPose()
{
	for(unsigned i=0; i<data->nbones; ++i) {
		auto& bone = data->bones[i];
		localAttitudes[i].radians = bone.radians;
		localAttitudes[i].scale = bone.scale;
		localTransforms[i] = bone.concatenatedMatrix();
	}
}

void Rig::computeWorldTransforms()
{
	// NOTE: SKIPPING ROOT
	
	// OPTIMIZATION CANDIDATES:
	// - maintain a dirty-mask over bones?
	
	for(unsigned i=1; i<data->nbones; ++i) {
		worldTransforms[i] = worldTransforms[data->bones[i].parentIndex] * localTransforms[i];
	}
	xformDirty = false;
}

