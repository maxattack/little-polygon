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

#pragma once
#include "sprites.h"
#include "collections.h"
#include "utils.h"

#define kTimelineTranslation   1
#define kTimelineRotation      2
#define kTimelineScale         3

//------------------------------------------------------------------------------
// ASSETS

struct RigTransform
{
	Vec2 translation;
	Vec2 scale;
	float radians;
	
	AffineMatrix concatenatedMatrix() const {
		auto uv = unitVector(radians);
		return AffineMatrix(scale.x * uv, scale.y * uv.anticlockwise(), translation);
	}
};

struct RigBoneAsset
{
	uint32_t parentIndex;
	uint32_t hash;
	RigTransform defaultTransform;
};

struct RigSlotAsset
{
	uint32_t boneIndex;
	uint32_t defaultAttachment;
	Color defaultColor;
};

struct RigAttachmentAsset
{
	RigSlotAsset *slot;
	ImageAsset *image;
	uint32_t hash;
	uint32_t layerHash;
	AffineMatrix xform;
};

struct RigAnimationAsset
{
	uint32_t hash;
	float duration;
};

struct RigTimelineAsset
{
	float *times;
	union {
		float *rotationValues;
		Vec2 *translationValues;
		Vec2 *scaleValues;
		int *attachmentValues;
	};
	uint32_t nkeyframes;
	uint32_t animHash;
	union {
		uint32_t boneIndex;
		uint32_t slotIndex;
	};
	uint32_t kind;
};

struct RigAsset
{
	uint32_t defaultLayer;
	uint32_t nbones;
	uint32_t nslots;
	uint32_t nattachments;
	uint32_t nanims;
	uint32_t ntimeslines;
	RigBoneAsset *bones;
	RigSlotAsset *slots;
	RigAttachmentAsset *attachments;
	RigAnimationAsset *anims;
	RigTimelineAsset *timelines;
};

//------------------------------------------------------------------------------
// RUNTIME CONTROLLER

class Rig {
private:
	const RigAsset* data;
	
	struct Attitude {
		float radians;
		Vec2 scale;
	};
	
	// Structure-of-Arrays:
	
	// (indexed by bone)
	Attitude* localAttitudes;
	AffineMatrix* localTransforms;
	AffineMatrix** parents;
	AffineMatrix* worldTransforms;
	
	// (indexed by timeline)
	BitArray timelineMask;
	int* currentKeyframes;
	
	uint32_t currentLayer;
	uint32_t currentAnimation;
	float currentTime;
	
	
public:
	Rig(const RigAsset* asset);
	~Rig();
	
	void setRootTransform(const AffineMatrix& mat);
	
	const AffineMatrix* findTransform(const char* boneName) const {
		return findTransform(fnv1a(boneName));
	}
	
	void draw(SpritePlotter* plotter);
	
	void setAnimation(const char *animName) {
		setAnimation(fnv1a(animName));
	}
	
	void tick(float dt);
	void resetPose();
	
	
private:
	
	const AffineMatrix* findTransform(uint32_t hash) const;
	void computeWorldTransforms();

	void setAnimation(uint32_t hash);
	
};

