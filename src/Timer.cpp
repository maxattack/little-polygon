#include <littlepolygon_events.h>

void TimerListener::clear() {
	if (isQueued()) {
		
		if (this == queue->listeners) {
			queue->listeners = next;
		} else {
			auto p = queue->listeners;
			while (p->next != this) { p = p->next; }
			p->next = next;
		}
		next = 0;
		queue = 0;
		
	}
}

void TimerQueue::clear() {
	while(listeners) {
		auto p = listeners;
		listeners = listeners->next;
		p->next = 0;
		p->queue = 0;
	}
}

void TimerQueue::enqueue(TimerListener *newListener, double duration) {
	ASSERT(!newListener->isQueued());
	newListener->time = time + duration;
	if (listeners == 0 || listeners->time > newListener->time) {
		newListener->next = listeners;
		listeners = newListener;
	} else {
		auto p = listeners;
		while(p->next && p->next->time < newListener->time) {
			p = p->next;
		}
		newListener->next = p->next;
		p->next = newListener;
	}
}

void TimerQueue::tick(double dt) {
	time += dt;
	while(listeners && listeners->time <= time) {
		auto p = listeners;
		listeners = p->next;
		p->queue = 0;
		p->next = 0;
		p->callback();
	}
	if (!listeners) {
		time = 0;
	}
}
