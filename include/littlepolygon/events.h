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
#include "collections.h"

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
	
	// portable stubs for converting different kinds of callbacks into a
	// common standard call.  This are often optimized away by the compiler.
	
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
	
	// private constructor, so that clients have to initialize this with
	// the explicit builder methods
	
	Action(void *aThis, CallbackPtr aCallback) :
		mThis(aThis), mCallback(aCallback) {}
	
public:
	
	// explicit build methods
	
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
	
	// functor operators
	
	void operator()(Args&& ... args) { ASSERT(mThis); mCallback(mThis, args...); }
	operator bool() const { return mThis != 0; }
	bool operator!() const { return !(operator bool()); }

	bool operator==(const Action<Args...>& other) { return mThis == other.mThis && mCallback == other.mCallback; }
};


//------------------------------------------------------------------------------
// GENERIC MULTICAST EVENT DELEGATE
//------------------------------------------------------------------------------

// Forward declarations

template<typename... Args> class EventCallback;
template<typename... Args> class EventDispatcher;

// EventListener is a control block for a single logical listener.  Multiple
// registrations for the same callback require multiple control blocks since
// the are iterated using a simple doubly-linked list.

template<typename... Args>
class EventCallback {
public:
	typedef EventCallback<Args...>* SiblingLink;
	typedef Action<Args...> Delegate;

	friend class EventDispatcher<Args...>;
	Delegate callback;

protected:
	SiblingLink prev, next;
	
public:
	EventCallback(Delegate aCallback) :
		callback(aCallback), prev(this), next(this) {
		
		}

	~EventCallback() { unbind(); }
	
	inline bool isBound() const { return prev != this; }

	void attachAfter(EventCallback *before) {
		ASSERT(prev == this);
		ASSERT(next == this);
		next = before->next;
		prev = before;
		before->next = this;
		next->prev = this;
	}
	
	void attachBefore(EventCallback *after) {
		ASSERT(prev == this);
		ASSERT(next == this);
		next = after;
		prev = after->prev;
		after->prev = this;
		prev->next = this;
	}
	
	void unbind() {
		ASSERT(next != 0);
		ASSERT(prev != 0);
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
public:
	typedef EventCallback<Args...> Listener;

private:
	Listener head;

public:
	EventDispatcher() : head(Listener::Delegate::none()) {}
	~EventDispatcher() { unbind(); }

	void bind(EventCallback<Args...>* listener) {
		
		// always add to the head of the list, so that new event subscriptions
		// made as a side-effect of emit() are not invoked for that dispatch.
		
		ASSERT(!listener->isBound());
		listener->attachAfter(&head);
		
	}

	inline bool isBound() { return head.isBound(); }
	void unbind() { while(isBound()) { head.next->unbind(); } }

	void emit(Args... args) {
		
		// we use a bookmark to allow events to be unsubscribed gracefully
		// without losing our place in the list (since the bookmark will not
		// be removed by side-effects).
		
		Listener bookmark(Listener::Delegate::none());
		auto p = head.next;
		while(p != &head) {
			bookmark.attachAfter(p);
			p->callback(std::forward<Args>(args) ...);
			p = bookmark.next;
			bookmark.unbind();
		}
		
	}

};

//------------------------------------------------------------------------------
// GENERIC TIMER CALLBACK MULTIPLEXOR
//------------------------------------------------------------------------------

// Timer listeners are a special event listener that carries a timeout which
// the timer queue uses to sort listeners.

class TimerCallback : public EventCallback<> {
friend class TimerQueue;
private:
	lpFloat time;
	
public:
	TimerCallback(Action<> aCallback) : EventCallback<>(aCallback), time(0.0f) {
		ASSERT(next == this);
		ASSERT(prev == this);
	}
	
	lpFloat nextTime() const { return static_cast<TimerCallback*>(next)->time; }
	lpFloat prevTime() const { return static_cast<TimerCallback*>(prev)->time; }
};

// Multiplexes several timeouts to deduplicate timeout work and simply defered
// callbacks.  Different timer queues could be used

class TimerQueue {
private:
	lpFloat time;
	TimerCallback head;
	
public:
	TimerQueue() : time(0.0f), head(Action<>::none()) {}
	~TimerQueue() { clear(); }
	
	inline bool hasQueue() const { return head.isBound(); }

	void clear() { while(hasQueue()) { head.next->unbind(); } }
	void enqueue(TimerCallback* newListener, lpFloat duration);
	void tick(lpFloat dt);
};

//------------------------------------------------------------------------------
// GENERIC DEFERED ACTION QUEUE
//------------------------------------------------------------------------------

class DeferredActionQueue {
private:
	List< Action<> > actions;
	
public:
	DeferredActionQueue() : actions(1024) {}
	
	void addAction(Action<> action) {
		if (actions.findFirst(action) == -1) {
			actions.append(action);
		}
	}
	
	void flush() {
		for(int i=0; i<actions.count(); ++i) {
			actions[i]();
		}
		actions.clear();
	}
};


