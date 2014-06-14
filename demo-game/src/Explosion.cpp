#include "game.h"

Explosion::Explosion(lpVec pos, float delay) :
position(pos),
time(-delay)
{
}

bool Explosion::tick() {
	time += lpTimer.deltaSeconds * 24.0f;
	return int(time) < gWorld.explosionImage->nframes;
}

void Explosion::draw() {
	if (time >= 0.0f) {
		lpSprites.drawImage(gWorld.explosionImage, position, int(time));
	}
}
