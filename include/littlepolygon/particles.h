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
	Color c0, c1;
	
public:
	Particle(lpFloat at0, lpFloat at1, Vec2 ap, Vec2 av, Color ca0, Color ac1);
	
	Vec2 position() const { return pos; }
	Vec2 velocity() const { return vel; }
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
	Vec2 position;
	lpFloat rate;
	lpFloat radius;
	lpFloat speedMin, speedMax;
	lpFloat angle, fov;
	lpFloat timeout;
	Color c0, c1;
	
public:
	ParticleEmitter(Vec2 p, lpFloat aRate);
	
	ParticleEmitter* setPosition(Vec2 p);
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
	lpFloat lifespan;
	Vec2 gravity;
	
	Pool<ParticleEmitter> emitters;
	CompactPool<Particle> particles;
	
	
public:
	ParticleSystem(lpFloat life=1.0, Vec2 g=vec(0,0));
	
	int count() const { return particles.size(); }
	
	void setLifespan(lpFloat life) { ASSERT(life > 0.0f); lifespan = life; }
	void setGravity(Vec2 g) { gravity = g; }
	
	ParticleEmitter* addEmitter(Vec2 p, lpFloat rate) { return emitters.alloc(p, rate); }
	void release(ParticleEmitter* emitter) { emitters.release(emitter); }
	
	void tick(lpFloat dt);
	void draw(SpritePlotter* plotter, ImageAsset *image);
	
};

