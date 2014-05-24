#include "game.h"



Hero::Hero() :
Entity(
	gAssets.userdata("hero.position")->get<vec2>(),
	vec(HeroWidth, HeroHeight)
),
img(gAssets.image("hero"))
{
}

void Hero::tick() {
	
}

void Hero::draw() {
	gSprites.drawImage(img, PixelsPerMeter * position());
}