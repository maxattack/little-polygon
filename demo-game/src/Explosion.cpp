#include "game.h"

Explosion::Explosion(Vec2 pos, float delay) :
position(pos),
time(-delay),
playedSfx(false)
{
}

bool Explosion::tick() {
	time += lpTimer.deltaSeconds * 24.0f;
	if (!playedSfx && time >= 0.0f) {
		playedSfx = true;
		lpAssets.sample("explosionSfx")->play();
	}
	return int(time) < gWorld.explosionImage->nframes;
}

void Explosion::draw() {
	if (time >= 0.0f) {
		lpSprites.drawImage(gWorld.explosionImage, position, int(time));
	}
}
