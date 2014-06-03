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
#include "pools.h"

class Particle {
private:
	float t;
	Vec2 pos;
	Vec2 vel;
	
public:
	Particle(float t, Vec2 pos, Vec2 vel);
	
	float time() const { return t; }
	Vec2 position() const { return pos; }
	Vec2 velocity() const { return vel; }
	
	void tick();
};

class ParticleSystem {
friend class Particle;
private:
	Vec2 emitterPosition;
	float emissionRadius;
	float emissionRate;
	float lifespans;
	Vec2 gravity;

	CompactPool<Particle> particles;
	
	
public:
	ParticleSystem(float radius, float rate, float lifespan, Vec2 g);
	
	void setPosition(Vec2 p);
	
	void tick(float dt);
	void draw(SpritePlotter* plotter, ImageAsset *image, Vec2 position);
	
};

