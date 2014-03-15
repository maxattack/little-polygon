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


#pragma once

// For ASSERT
#include "littlepolygon_base.h"

//------------------------------------------------------------------------------
// GENERIC CALLBACK DELEGATE
//------------------------------------------------------------------------------

// C++11 Update to old MIT-licensed generic callback:
// http://www.codeproject.com/Articles/11015/The-Impossibly-Fast-C-Delegates

template<typename... Args>
class Action {
private:
	typedef void (*CallbackPtr)(void* aThis, Args...);
	void *mThis;
	CallbackPtr mCallback;
	
	template<void (*TFunc)(Args...)>
	static void funcStub(Args... args) { TFunc(args...); }
	
	template<class T, void (T::*TMethod)(Args...)>
	static void methodStub(void *aThis, Args... args) {
		T* p = static_cast<T*>(aThis);
		(p->*TMethod)(args...);
	}
	
	template<class T, void (T::*TMethod)(Args...) const>
	static void constMethodStub(void *aThis, Args... args) {
		T const* p = static_cast<T*>(aThis);
		(p->*TMethod)(args...);
	}
	
	Action(void *aThis, CallbackPtr aCallback) :
		mThis(aThis), mCallback(aCallback) {}
	
public:
	
	static Action none() { return Action(0, 0); }
	
	template<class T, void (T::*TMethod)(Args...)>
	static Action callMethod(T* context) {
		return Action(context, &methodStub<T, TMethod>);
	}
	
	template<void (*TFunc)(Args...)>
	static Action callFunc() {
		return Action(0, &funcStub<TFunc>);
	}
	
	template<class T, void (T::*TMethod)(Args...) const>
	static void callConstMethod(T* context) {
		return Action(const_cast<T*>(context), &constMethodStub<T, TMethod>);
	}
	
	void operator()(Args&& ... args) {
		ASSERT(mThis);
		mCallback(mThis, args...);
	}
	
	operator bool() const {
		return mThis != 0;
	}
	
	bool operator!() const
	{
		return !(operator bool());
	}
};


//------------------------------------------------------------------------------
// GENERIC MULTICAST EVENT DELEGATE
//------------------------------------------------------------------------------

// Forward declarations
template<typename... Args> class EventListener;
template<typename... Args> class EventDispatcher;

// EventListener is a control block for a single logical listener.  Multiple
// registrations for the same callback require multiple control blocks since
// the are iterated using a simple doubly-linked list.
template<typename... Args>
class EventListener {
friend class EventDispatcher<Args...>;
public:
	typedef EventListener<Args...>* SiblingLink;
	typedef Action<Args...> Delegate;

private:
	Delegate callback;
	SiblingLink prev, next;
	
	void attachAfter(EventListener *before) {
		next = before->next;
		prev = before;
		before->next = this;
		next->prev = this;
	}
	
public:
	// Probably makes sense to typedef this as a concrete instantiation to
	// make it easier to type.
	EventListener(Delegate aCallback) :
		callback(aCallback), prev(this), next(this) {}
	~EventListener() { unbind(); }
	
	inline bool isBound() const { return next != this; }

	void unbind() {
		next->prev = prev;
		prev->next = next;
		next = this;
		prev = this;
	}
};

// EventDispatcher is an opaque dispatcher that can be allocated in place as a
// public parameters without leaking implementation details (unless you don't
// want "emit" to be public).
template<typename... Args>
class EventDispatcher {
friend class EventListener<Args...>;
public:
	typedef EventListener<Args...> Listener;

private:
	Listener head;

public:
	EventDispatcher() : head(Listener::Delegate::none()) {}
	~EventDispatcher() { unbind(); }

	void bind(EventListener<Args...>* listener) {
		ASSERT(!listener->isBound());
		// always add to the head of the list, so that new event subscriptions
		// made as a side-effect of emit() are not invoked for that dispatch.
		listener->attachAfter(&head);
	}

	void unbind() {
		while(isBound()) { head.next->unbind(); }
	}

	inline bool isBound() { return head.isBound(); }

	// It's OK to add/remove listeners while the event is being emitted. New
	// listeners will not be triggered by the current event.
	void emit(Args&&... args) {
		// we use a bookmark to allow events to be unsubscribed gracefully
		// without losing our place in the list (since the bookmark will not
		// be removed by side-effects).
		Listener bookmark(Listener::Delegate::none());
		auto p = head.next;
		while(p != &head) {
			bookmark.attachAfter(p);
			p->callback(args...);
			p = bookmark.next;
			bookmark.unbind();
		}
	}

};

//------------------------------------------------------------------------------
// GENERIC TIMER CALLBACK MULTIPLEXOR
//------------------------------------------------------------------------------

class TimerListener;
class TimerQueue;

class TimerListener {
public:
	friend class TimerQueue;

private:
	Action<> callback;
	TimerQueue *queue;
	double time;
	TimerListener *next;

public:
	TimerListener(Action<> aCallback) :
		callback(aCallback), queue(0), time(0), next(0) {}
	~TimerListener() { clear(); }
	
	
	inline bool isQueued() const { return queue != 0; }
	
	void clear();
};

class TimerQueue {
private:
	friend class TimerListener;
	double time;
	TimerListener *listeners;
	
public:
	TimerQueue() : time(0), listeners(0) {}
	~TimerQueue() { clear(); }
	
	inline bool hasQueue() const { return listeners != 0; }
	void clear();
	
	void enqueue(TimerListener* newListener, double duration);
	void tick(double dt);
};


