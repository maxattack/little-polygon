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

#include <littlepolygon/events.h>

void TimerQueue::enqueue(TimerListener *newListener, double duration) {
	ASSERT(!newListener->isBound());
	if (hasQueue()) {
		
		// we need to determine if we're appending to the end of the list (common case)
		// or if we need to find a spot in the middle
		
		newListener->time = duration+time;
		if (head.prevTime() < newListener->time) {
			newListener->attachBefore(&head);
		} else {
			auto p = &head;
			while(p->nextTime() < newListener->time) {
				p = static_cast<TimerListener*>(p->next);
			}
			newListener->attachAfter(p);
		}
		
	} else {
		
		// the first listener resets the timer (staying near zero feels like
		// a good idea over the long term :P)
		
		time = 0;
		newListener->time = duration;
		newListener->attachAfter(&head);
		
	}
		
}

void TimerQueue::tick(double dt) {
	if (hasQueue()) {
		
		// dequeue everything that's passed below the time threshold
		
		time += dt;
		while(hasQueue() && head.nextTime() <= time) {
			auto p = head.next;
			p->unbind();
			p->callback();
		}
		
	}
}
