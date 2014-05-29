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
#include "utils.h"

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

struct RigTimeline
{
	float *times;
	float *values;
	uint32_t nkeyframes;
	uint32_t animHash;
	uint32_t boneIndex;
};

struct RigAsset
{
	uint32_t defaultLayer;
	uint32_t nbones;
	uint32_t nslots;
	uint32_t nattachments;
	RigBoneAsset *bones;
	RigSlotAsset *slots;
	RigAttachmentAsset *attachments;
};

//------------------------------------------------------------------------------
// RUNTIME CONTROLLER

class Rig {
private:
	const RigAsset* mAsset;
	AffineMatrix* mLocalTransforms;
	AffineMatrix** mParents;
	AffineMatrix* mWorldTransforms;
	
	uint32_t currentLayer;
	
	
public:
	Rig(const RigAsset* asset);
	~Rig();
	
	void setRootTransform(const AffineMatrix& mat);
	
	const AffineMatrix* findTransform(const char* boneName) const {
		return findTransform(fnv1a(boneName));
	}
	const AffineMatrix* findTransform(uint32_t hash) const;
	
	void draw(SpritePlotter* plotter);
	
private:
	
	void computeWorldTransforms();

	
};

