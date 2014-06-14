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

#include "base.h"
#include <utility>

//------------------------------------------------------------------------------
// POOL
// Allocates objects in arrays with a doubling strategy, and stores active/free
// objects in linked lists.

template<typename T>
class Pool {
private:
	static const int kDefaultReserve = 1024;
	static const int kBufferCapacity = 8;
	typedef Linkable<T> Slot;
	unsigned bufferCount, bufferSize;
	Link active, idle;
	Slot *buffers[kBufferCapacity];
	
public:
	Pool(unsigned reserve=0) : bufferCount(0), bufferSize(0)
	{
		memset(buffers, 0, kBufferCapacity * sizeof(Slot*));
		if (reserve) { allocBuffer(reserve); }
	}
	
	~Pool()
	{
		clear();
		while(active.isBound()) {
			Slot::getValue(active.next)->~T();
			active.next->unbind();
		}
		idle.unbind();
		for(unsigned i=0; i<bufferCount; ++i) {
			lpFree(buffers[i]);
		}
	}
	
	bool isEmpty() const
	{
		return !active.isBound();
	}
	
	void clear()
	{
		while(active.isBound()) {
			release(Slot::getValue(active.next));
		}
	}
	
	template<typename... Args>
	T* alloc(Args&&... args)
	{
		if (!idle.isBound()) {
			allocBuffer(bufferSize > 0 ? (bufferSize + bufferSize) : kDefaultReserve);
		}
		auto link = idle.next;
		link->unbind();
		link->attachAfter(&active);
		return new(Slot::getValue(link)) T(std::forward<Args>(args) ...);
	}
	
	void release(T* p)
	{
		auto link = Slot::getLink(p);
		ASSERT(link->isBound());
		p->~T();
		link->unbind();
		link->attachAfter(&idle);
	}
	
	class Iterator {
	private:
		Link* head;
		Link bookmark;
		
	public:
		Iterator(Link* aHead) : head(aHead) {
			bookmark.attachAfter(head);
		}
		
		
		T* operator->() { return Slot::getValue(bookmark.prev); }
		T& operator*() { return *Slot::getValue(bookmark.prev); }
		
		bool next()
		{
			auto p = bookmark.next;
			bookmark.unbind();
			if (p == head) {
				return false;
			} else {
				bookmark.attachAfter(p);
				return true;
			}
		}
	};
	
	Iterator list() { return &active; }
	
	template<typename T0, typename ...Args>
	void cull(bool (T0::*Func)(Args...), Args... args)
	{
		Link bookmark;
		auto p = active.next;
		while(p != &active) {
			bookmark.attachAfter(p);
			auto ptr = Slot::getValue(p);
			if ((ptr->*Func)(args...)) { release(ptr); }
			p = bookmark.next;
			bookmark.unbind();
		}
			
	}

private:
	void allocBuffer(unsigned size) {
		ASSERT(bufferCount < kBufferCapacity);
		bufferSize = size;
		auto buf = buffers[bufferCount] = (Slot*) lpCalloc(size, sizeof(Slot));
		for(unsigned i=0; i<size; ++i) {
			Link* l = new(static_cast<Link*>(buf+i)) Link();
			l->attachBefore(&idle);
		}
		++bufferCount;
	}
	
};


//------------------------------------------------------------------------------
// COMPACT POOL
// Faster iteration and no upper-bound on size, but records are not guarenteed
// to remain at fixed memory addresses (either because of reallocation or
// swapping-with-end on dealloc).  Designed for "anonymous collections" like
// particle systems.
//------------------------------------------------------------------------------

template<typename T, bool kGrow=true>
class CompactPool {
public:
	static const int kDefaultReserve = 1024;
	typedef T InstanceType;
	
private:
	int count, capacity;
	T* slots;

public:
	
	CompactPool(unsigned reserve=kDefaultReserve) : count(0), capacity(reserve), slots(0) {
		ASSERT(capacity > 0);
	}
	
	~CompactPool() {
		clear();
		lpFree(slots);
	}
	
	bool isEmpty() const { return count == 0; }
	int size() const { return count; }
	bool active(const T* p) const { return p >= slots && p < slots + count; }

	T* begin() { return slots; }
	T* end() { return slots + count; }
	const T* begin() const { return slots; }
	const T* end() const { return slots + count; }
	
	void clear()
	{
		if (slots) {
			while(count > 0) { release(&slots[count-1]); }
		}
	}

	template<typename... Args>
	T* alloc(Args&&... args)
	{
		if (slots == 0) {
			slots = (T*) lpCalloc(capacity, sizeof(T));
		} else if (count == capacity) {
			ASSERT(kGrow);
			capacity += capacity;
			slots = (T*) lpRealloc(slots, capacity * sizeof(T));
		}
		++count;
		return new (&slots[count-1]) T(std::forward<Args>(args)...);
	}

	void release(T* p)
	{
		ASSERT(active(p));
		p->~T();
		--count;
		if (p != slots + count) {
			memcpy(p, slots+count, sizeof(T));
		}
	}
	
	template<typename T0, typename ...Args>
	void cull(bool (T0::*Func)(Args...), Args... args)
	{
		for(int i=0; i<count;) {
			if ((slots[i].*Func)(args...)) { release(slots+i); } else { ++i; }
		}
	}
	
};
