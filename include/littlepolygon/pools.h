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

// Several different common/simple object pools that index into a linear
// array of preallocated slots.  Objects in the pool are not automatically
// finalized with the pool, so make sure to explicitly clear() them if the
// destructors matter.

#include "collections.h"
#include <vector>

//------------------------------------------------------------------------------
// POOL
// Allocates objects in arrays with a doubling strategy.  Doesn't allocate
// anything if you never use it.
//------------------------------------------------------------------------------

struct Link {
	Link *prev, *next;
	
	void init() {
		next = this;
		prev = this;
	}
	
	void attachAfter(Link *before) {
		next = before->next;
		prev = before;
		before->next = this;
		next->prev = this;
	}
	
	void attachBefore(Link *after) {
		next = after;
		prev = after->prev;
		after->prev = this;
		prev->next = this;
	}
	
	void unbind() {
		next->prev = prev;
		prev->next = next;
		next = this;
		prev = this;
	}
	
	bool isBound() const {
		return next != this;
	}
};

template<typename T>
class Pool {
public:
	typedef T InstanceType;
	
private:
	// NOTHING TO SEE HERE, MOVE ALONG :P
	struct PoolSlot : Link { Slot<T> storage; };
	static Slot<T>& getStorage(Link* link) { return static_cast<PoolSlot*>(link)->storage; }
	static PoolSlot* getSlot(T* p) { return static_cast<PoolSlot*>(static_cast<Link*>((void*)p)-1); }
	
	unsigned bufferCount;
	Link active, idle, bookmark;
	PoolSlot *buffers[7];
	
public:
	Pool() : bufferCount(0)
	{
		memset(buffers, 0, 7 * sizeof(PoolSlot*));
		active.init();
		idle.init();
		bookmark.init();
	}
	
	~Pool()
	{
		for(int i=0; i<7; ++i) {
			free(buffers[i]);
		}
	}
	
	bool isEmpty() const { return !active.isBound(); }
	
	void clear()
	{
		while(active.isBound()) {
			release(getStorage(active.next).address());
		}
	}
	
	template<typename... Args>
	T* alloc(Args&&... args)
	{
		if (!idle.isBound()) {
			ASSERT(bufferCount < (7-1));
			int cnt = 8<<bufferCount;
			auto buf = buffers[bufferCount] = (PoolSlot*) calloc(cnt, sizeof(PoolSlot));
			for(int i=0; i<cnt; ++i) {
				buf[i].init();
				buf[i].attachBefore(&idle);
			}
			++bufferCount;
		}
		
		auto slot = idle.next;
		slot->unbind();
		slot->attachAfter(&active);
		return new(getStorage(slot).address()) T(std::forward<Args>(args) ...);
	}
	
	void release(T* p)
	{
		auto slot = getSlot(p);
		ASSERT(slot->isBound());
		getStorage(slot).release();
		slot->unbind();
		slot->attachAfter(&idle);
	}
	
	void each(void (T::*f)())
	{
		auto p = active.next;
		while(p != &active) {
			bookmark.attachAfter(p);
			(getStorage(p).reference().*f)();
			p = bookmark.next;
			bookmark.unbind();
		}
	}
	
	void cull(bool (T::*f)())
	{
		auto p = active.next;
		while(p != &active) {
			bookmark.attachAfter(p);
			auto ptr = getStorage(p).address();
			if ((ptr->*f)()) { release(ptr); }
			p = bookmark.next;
			bookmark.unbind();
		}
			
	}

};


//------------------------------------------------------------------------------
// COMPACT POOL
// Faster iteration and no upper-bound on size, but records are not guarenteed
// to remain at fixed memory addresses (either because of reallocation or
// swapping-with-end on dealloc).  Designed for "anonymous collections" like
// particle systems.
//------------------------------------------------------------------------------

template<typename T>
class CompactPool {
public:
	typedef T InstanceType;
	
private:
	// TODO: CONSIDER A NON-STL BACKING STORE :P
	std::vector<Slot<T> > slots;

public:
	
	CompactPool() {}
	
	inline bool isEmpty() const { return slots.empty(); }
	inline bool active(const T* p) const { return p >= slots.data()->address() && p < slots.data()->address() + slots.size(); }
	inline T* begin() { return slots.data()->address(); }
	inline T* end() { return begin() + slots.size(); }
	
	void clear()
	{
		for(auto& slot : slots) { slot.release(); }
		slots.clear();
	}

	template<typename... Args>
	T* alloc(Args&&... args)
	{
		slots.emplace_back();
		return slots.back().init(args...);
	}

	void release(T* p)
	{
		ASSERT(active(p));
		p->~T();
		if (p != slots.back().address()) {
			*p = slots.back().value();
		}
		slots.pop_back();
	}
	
	void each(void (T::*Func)())
	{
		for(auto inst=begin(); inst!=end(); ++inst) {
			(inst->*Func)();
		}
	}

	
	void cull(bool (T::*Func)())
	{
		for(auto inst=begin(); inst!=end();) {
			if ((inst->*Func)()) { release(inst); } else { ++inst; }
		}
	}
	
};
