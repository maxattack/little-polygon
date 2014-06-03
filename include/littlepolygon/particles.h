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
	lpFloat t0, t1;
	Vec2 pos;
	Vec2 vel;
	
public:
	Particle(lpFloat at0, lpFloat at1, Vec2 ap, Vec2 av);
	
	Vec2 position() const { return pos; }
	Vec2 velocity() const { return vel; }
	lpFloat startTime() const { return t0; }
	lpFloat endTime() const { return t1; }
	lpFloat lifespan() const { return t1 - t0; }
	
	bool tick(const ParticleSystem* sys, lpFloat dt);
};

class ParticleSystem {
friend class Particle;
private:

	// user-settable parameters
	bool emitting;
	Vec2 emitterPosition;
	lpFloat emissionRadius;
	lpFloat emissionRate;
	lpFloat emitSpeedMin;
	lpFloat emitSpeedMax;
	lpFloat emitAngleMin;
	lpFloat emitAngleMax;
	lpFloat lifespan;
	Vec2 gravity;
	
	// render parameters
	Color startColor;
	Color endColor;
	Color startMod;
	Color endMod;
	
	// simulation parameters
	lpFloat time;
	lpFloat emitCount;
	
	// particle instances
	CompactPool<Particle> particles;
	
	
public:
	ParticleSystem();
	
	void setEmitting(bool flag);
	void setEmitterPosition(Vec2 p);
	void setEmissionRadius(lpFloat r);
	void setEmissionRate(lpFloat rate);
	void setEmissionSpeed(lpFloat min, lpFloat max);
	void setEmissionRadians(lpFloat min, lpFloat max);
	void setLifespan(lpFloat life);
	void setGravity(Vec2 g);
	
	void setColorAdd(Color c0, Color c1);
	void setColorMod(Color c0, Color c1);
	
	void emit(int n);
	void tick(lpFloat dt);
	void draw(SpritePlotter* plotter, ImageAsset *image);
	
};

