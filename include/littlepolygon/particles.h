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

// A super-simple particle system which is more like a reference implementation
// than an extensible tool.

class ParticleSystem;

class Particle {
private:
	float t0, t1;
	Vec2 pos;
	Vec2 vel;
	
public:
	Particle(float at0, float at1, Vec2 ap, Vec2 av);
	
	Vec2 position() const { return pos; }
	Vec2 velocity() const { return vel; }
	float startTime() const { return t0; }
	float endTime() const { return t1; }
	float lifespan() const { return t1 - t0; }
	
	bool tick(const ParticleSystem* sys, float dt);
};

class ParticleSystem {
friend class Particle;
private:

	// user-settable parameters
	bool emitting;
	Vec2 emitterPosition;
	float emissionRadius;
	float emissionRate;
	float emitSpeedMin;
	float emitSpeedMax;
	float emitAngleMin;
	float emitAngleMax;
	float lifespan;
	Vec2 gravity;
	
	// render parameters
	Color startColor;
	Color endColor;
	Color startMod;
	Color endMod;
	
	// simulation parameters
	float time;
	float emitCount;
	
	// particle instances
	CompactPool<Particle> particles;
	
	
public:
	ParticleSystem();
	
	void setEmitting(bool flag);
	void setEmitterPosition(Vec2 p);
	void setEmissionRadius(float r);
	void setEmissionRate(float rate);
	void setEmissionSpeed(float min, float max);
	void setEmissionRadians(float min, float max);
	void setLifespan(float life);
	void setGravity(Vec2 g);
	
	void setColorAdd(Color c0, Color c1);
	void setColorMod(Color c0, Color c1);
	
	void emit(int n);
	void tick(float dt);
	void draw(SpritePlotter* plotter, ImageAsset *image);
	
};

