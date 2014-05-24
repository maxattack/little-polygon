#include "game.h"


Kitten::Kitten() :
Entity(
	gAssets.userdata("kitten.position")->get<vec2>(),
	vec(KittenWidth, KittenHeight)
),
img(gAssets.image("kitten"))
{
}

void Kitten::tick() {
	
}

void Kitten::draw() {
	gSprites.drawImage(img, PixelsPerMeter * position());
}
