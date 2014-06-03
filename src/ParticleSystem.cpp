#include "littlepolygon/particles.h"

Particle::Particle(float at0, float at1, Vec2 ap, Vec2 av) :
t0(at0), t1(at1), pos(ap), vel(av)
{
	ASSERT(t1 > t0);
}

bool Particle::tick(const ParticleSystem *sys, float dt)
{
	if (sys->time < t1) {
		vel += dt * sys->gravity;
		pos += dt * vel;
		return false;
	} else {
		return true;
	}
}

ParticleSystem::ParticleSystem() :
emitterPosition(0,0),
emissionRadius(0),
emissionRate(0),
emitSpeedMin(0),
emitSpeedMax(0),
emitAngleMin(0),
emitAngleMax(kTAU),
lifespan(1.0f),
gravity(0,0),

startColor(rgba(0)),
endColor(rgba(0)),
startMod(rgba(0xffffffff)),
endMod(rgba(0xffffffff)),

time(0),
emitting(false),
emitCount(0.0f)
{
}

void ParticleSystem::setEmitting(bool flag)
{
	emitting = flag;
}

void ParticleSystem::setEmitterPosition(Vec2 p)
{
	emitterPosition = p;
}

void ParticleSystem::setEmissionRadius(float r)
{
	ASSERT(r >= 0.0f);
	emissionRadius = r;
}

void ParticleSystem::setEmissionRate(float rate)
{
	ASSERT(rate >= 0.0f);
	emissionRate = rate;
}

void ParticleSystem::setEmissionSpeed(float min, float max)
{
	emitSpeedMin = min;
	emitSpeedMax = max;
}

void ParticleSystem::setEmissionRadians(float min, float max)
{
	emitAngleMin = min;
	emitAngleMax = max;
}

void ParticleSystem::setLifespan(float life)
{
	ASSERT(life > 0.0f);
	lifespan = life;
}

void ParticleSystem::setGravity(Vec2 g)
{
	gravity = g;
}

void ParticleSystem::setColorAdd(Color c0, Color c1)
{
	startColor = c0;
	endColor = c1;
}

void ParticleSystem::setColorMod(Color c0, Color c1)
{
	startMod = c0;
	endMod = c1;
}


void ParticleSystem::emit(int n)
{
	while(n > 0) {
		particles.alloc(
			time, time+lifespan,
			emitterPosition + emissionRadius * randomPointInsideCircle(),
			polarVector(
				randomValue(emitSpeedMin, emitSpeedMax),
				randomValue(emitAngleMin, emitAngleMax)
			)
		);
		--n;
	}
}

void ParticleSystem::tick(float dt)
{
	if  (emitting) {
		emitCount += dt * emissionRate;
		int cnt = floorToInt(emitCount);
		if (cnt > 0) {
			emit(cnt);
			emitCount -= cnt;
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
	auto r = std::max(image->size.x,image->size.y);
	for(auto p=particles.begin(); p!=particles.end(); ++p) {
		if (plotter->viewport().contains(p->position(), r)) {
			auto u = (time - p->startTime()) / p->lifespan();
			plotter->drawImage(
				image,
				p->position(),
				0,
				lerp(startColor, endColor, u),
				lerp(startMod, endMod, u)
			);
		}
	}
}


