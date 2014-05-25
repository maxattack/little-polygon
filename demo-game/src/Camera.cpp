#include "game.h"

Camera::Camera() : quakeTime(-1), position(0,0) {
}

void Camera::quake() {
	quakeTime = 0.2f;
}

void Camera::tick() {
	if (quakeTime > 0.0f) {
		quakeTime -= gTimer.deltaSeconds;
		if (quakeTime <= 0.0f) {
			gView.setOffset(position);
		} else {
			gView.setOffset(position + vec(0, -10.0f * quakeTime));
		}
		
	}
}