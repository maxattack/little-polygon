#include "game.h"

Camera::Camera() :
quakeTime(-1),
flashTime(-1),
restColor(lpAssets.palette("global")->getColor(0)),
position(0,0)
{
	glClearColor(restColor.red(), restColor.green(), restColor.blue(), 0);
}

void Camera::quake() {
	quakeTime = 0.15f;
}

void Camera::flash() {
	glClearColor(1, 1, 1, 1);
	flashTime = 0.115f;
}

void Camera::tick() {
	if (quakeTime > 0.0f) {
		quakeTime -= lpTimer.deltaSeconds;
		if (quakeTime <= 0.0f) {
			lpView.setOffset(position);
		} else {
			lpView.setOffset(position + vec(0, -20.0f * quakeTime));
		}
		
	}
	
	if (flashTime > 0.0f) {
		flashTime -= lpTimer.deltaSeconds;
		if (flashTime <= 0.0f) {
			glClearColor(restColor.red(), restColor.green(), restColor.blue(), 0);
			quake();
		}
	}
}