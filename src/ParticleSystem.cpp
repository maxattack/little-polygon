#include "littlepolygon/particles.h"

//--------------------------------------------------------------------------------

Particle::Particle(lpFloat at0, lpFloat at1, lpVec ap, lpVec av, Color ac0, Color ac1) :
t0(at0), t1(at1), pos(ap), vel(av), c0(ac0), c1(ac1)
{
	ASSERT(t1 > t0);
}

bool Particle::tick(const ParticleSystem *sys, lpFloat dt)
{
	if (sys->time < t1) {
		vel += dt * sys->gravity;
		pos += dt * vel;
		return false;
	} else {
		return true;
	}
}

//--------------------------------------------------------------------------------

ParticleEmitter::ParticleEmitter(lpVec p, lpFloat aRate) :
position(p),
rate(aRate),
radius(0.0f),
speedMin(0.0f),
speedMax(0.0f),
angle(0.0f),
fov(kTAU),
lifespan(1.0),
timeout(expovariate(1.0f/rate)),
c0(rgba(0xffffffff)),
c1(rgba(0xffffff00))
{
}

ParticleEmitter* ParticleEmitter::setLifespan(lpFloat life)
{
	ASSERT(life > 0.0f);
	lifespan = life;
	return this;
}

ParticleEmitter* ParticleEmitter::setPosition(lpVec p)
{
	position = p;
	return this;
}

ParticleEmitter* ParticleEmitter::setRate(lpFloat aRate)
{
	rate = aRate;
	return this;
}

ParticleEmitter* ParticleEmitter::setRadius(lpFloat aRadius)
{
	radius = aRadius;
	return this;
}

ParticleEmitter* ParticleEmitter::setSpeed(lpFloat aMin, lpFloat aMax)
{
	speedMin = aMin;
	speedMax = aMax;
	return this;
}

ParticleEmitter* ParticleEmitter::setAngle(lpFloat aAngle, lpFloat aFov)
{
	angle = aAngle;
	fov = 0.5f * aFov;
	return this;
}

ParticleEmitter* ParticleEmitter::setColor(Color ac0, Color ac1)
{
	c0 = ac0;
	c1 = ac1;
	return this;
}

//--------------------------------------------------------------------------------

ParticleSystem::ParticleSystem(lpVec g) :
time(0),
gravity(g),
particles(1024)
{
}

void ParticleSystem::tick(lpFloat dt)
{
	time += dt;
	
	// tick emitters
	for(emitters.iterBegin(); auto e=emitters.iterNext();) {
		e->timeout -= dt;
		while (e->timeout < 0.0f) {
			e->timeout += expovariate(1.0f/e->rate);
			particles.alloc(
				time, time+e->lifespan,
				e->position + polarVector(
					(1.0f-easeOut2(randomValue())) * e->radius,
					randomValue(0, kTAU)
				),
				polarVector(
					randomValue(e->speedMin, e->speedMax),
					randomValue(e->angle - e->fov, e->angle + e->fov)
				), e->c0, e->c1
			);
		}
	}
	
	// particle values swap-with-the-end when they expire
	for(auto p=particles.begin(); p!=particles.end();) {
		if (p->tick(this, dt)) { particles.release(p); } else { ++p; }
	}
}

void ParticleSystem::draw(SpritePlotter* plotter, ImageAsset *image)
{
	// clip to onscreen particles
	auto r = MAX(image->size.x,image->size.y);
	for(auto p=particles.begin(); p!=particles.end(); ++p) {
		if (plotter->viewport().contains(p->position(), r)) {
			auto u = (time - p->startTime()) / p->lifespan();
			plotter->drawImage(
				image,
				p->position(),
				0, rgba(0),
				lerp(p->startColor(), p->endColor(), u)
			);
		}
	}
}


