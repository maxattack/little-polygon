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
