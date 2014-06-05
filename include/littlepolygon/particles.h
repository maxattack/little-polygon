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

/*

 A super-simple particle system which is more like a reference implementation
than an extensible tool.

GIFT IDEAS
- per emitter size?
- per emitter force?
- simple world-collision interface?
 
*/


class ParticleSystem;

class Particle {
private:
	lpFloat t0, t1;
	lpVec pos;
	lpVec vel;
	Color c0, c1;
	
public:
	Particle(lpFloat at0, lpFloat at1, lpVec ap, lpVec av, Color ca0, Color ac1);
	
	lpVec position() const { return pos; }
	lpVec velocity() const { return vel; }
	lpFloat startTime() const { return t0; }
	lpFloat endTime() const { return t1; }
	lpFloat lifespan() const { return t1 - t0; }
	Color startColor() const { return c0; }
	Color endColor() const { return c1; }
	
	bool tick(const ParticleSystem* sys, lpFloat dt);
};

class ParticleEmitter {
friend class ParticleSystem;
private:
	lpVec position;
	lpFloat rate;
	lpFloat radius;
	lpFloat speedMin, speedMax;
	lpFloat angle, fov;
	lpFloat lifespan;
	lpFloat timeout;
	Color c0, c1;
	
public:
	ParticleEmitter(lpVec p, lpFloat aRate);
	
	ParticleEmitter* setPosition(lpVec p);
	ParticleEmitter* setLifespan(lpFloat life);
	ParticleEmitter* setRate(lpFloat rate);
	ParticleEmitter* setRadius(lpFloat radius);
	ParticleEmitter* setSpeed(lpFloat min, lpFloat max);
	ParticleEmitter* setAngle(lpFloat angle, lpFloat fov);
	ParticleEmitter* setColor(Color ac0, Color ac1);
};

class ParticleSystem {
friend class Particle;
friend class ParticleEmitter;
private:

	lpFloat time;
	lpVec gravity;
	
	Pool<ParticleEmitter> emitters;
	CompactPool<Particle> particles;
	
	
public:
	ParticleSystem(lpVec g=vec(0,0));
	
	int count() const { return particles.size(); }
	
	void setGravity(lpVec g) { gravity = g; }
	
	ParticleEmitter* addEmitter(lpVec p, lpFloat rate) { return emitters.alloc(p, rate); }
	void release(ParticleEmitter* emitter) { emitters.release(emitter); }
	
	void tick(lpFloat dt);
	void draw(SpritePlotter* plotter, ImageAsset *image);
	
};

