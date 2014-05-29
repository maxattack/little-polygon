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
		return findTransform(AssetBundle::hash(boneName));
	}
	const AffineMatrix* findTransform(uint32_t hash) const;
	
	void draw(SpritePlotter* plotter);
	
private:
	
	void computeWorldTransforms();

	
};