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

//------------------------------------------------------------------------------
// POOL
// Allocates objects in arrays with a doubling strategy.  Doesn't allocate
// anything if you never use it.
//------------------------------------------------------------------------------

template<typename T>
class Pool {
public:
	static const unsigned kBufferCapacity = 8;
	static const unsigned kDefaultReserve = 8;
	typedef T InstanceType;
	typedef Slot<T> StorageType;
	
private:
	struct PoolSlot : Link, StorageType {};
	unsigned bufferCount, bufferSize;
	Link active, idle, bookmark;
	PoolSlot *buffers[kBufferCapacity];
	
public:
	Pool(unsigned reserve=0) : bufferCount(0), bufferSize(0)
	{
		memset(buffers, 0, kBufferCapacity * sizeof(PoolSlot*));
		active.initLink();
		idle.initLink();
		bookmark.initLink();
		if (reserve) { allocBuffer(reserve); }
	}
	
	~Pool()
	{
		for(unsigned i=0; i<bufferCount; ++i) {
			free(buffers[i]);
		}
	}
	
	bool isEmpty() const
	{
		return !active.isBound();
	}
	
	void clear()
	{
		while(active.isBound()) {
			release(getStorage(active.next));
		}
	}
	
	template<typename... Args>
	T* alloc(Args&&... args)
	{
		if (!idle.isBound()) {
			allocBuffer(bufferSize > 0 ? (bufferSize + bufferSize) : kDefaultReserve);
		}
		auto slot = idle.next;
		slot->unbind();
		slot->attachAfter(&active);
		return new(getStorage(slot)) T(std::forward<Args>(args) ...);
	}
	
	void release(T* p)
	{
		auto slot = getSlot(p);
		ASSERT(slot->isBound());
		getStorage(slot)->~T();
		slot->unbind();
		slot->attachAfter(&idle);
	}
	
	void each(void (Func)(T*))
	{
		auto p = active.next;
		while(p != &active) {
			bookmark.attachAfter(p);
			Func(getStorage(p));
			p = bookmark.next;
			bookmark.unbind();
		}
		
	}
	
	template<typename T0, typename ...Args>
	void each(void (T0::*Func)(Args...), Args... args)
	{
		auto p = active.next;
		while(p != &active) {
			bookmark.attachAfter(p);
			(getStorage(p)->*Func)(args...);
			p = bookmark.next;
			bookmark.unbind();
		}
	}
	
	template<typename T0, typename ...Args>
	void cull(bool (T0::*Func)(Args...), Args... args)
	{
		auto p = active.next;
		while(p != &active) {
			bookmark.attachAfter(p);
			auto ptr = getStorage(p);
			if ((ptr->*Func)(args...)) { release(ptr); }
			p = bookmark.next;
			bookmark.unbind();
		}
			
	}

private:
	void allocBuffer(unsigned size) {
		ASSERT(bufferCount < kBufferCapacity);
		bufferSize = size;
		auto buf = buffers[bufferCount] = (PoolSlot*) calloc(size, sizeof(PoolSlot));
		for(unsigned i=0; i<size; ++i) {
			buf[i].initLink();
			buf[i].attachBefore(&idle);
		}
		++bufferCount;
	}
	
	static T* getStorage(Link* link)
	{
		return static_cast<StorageType*>(static_cast<PoolSlot*>(link))->address();
	}
	
	static PoolSlot* getSlot(T* p)
	{
		return static_cast<PoolSlot*>(static_cast<StorageType*>((void*)p));
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
	static const int kDefaultReserve = 8;
	
private:
	int count, capacity;
	T* slots;

public:
	
	CompactPool(unsigned reserve=0) : count(0), capacity(reserve), slots(0) {
		if (reserve) {
			slots = (T*) calloc(reserve, sizeof(T));
		}
	}
	
	~CompactPool() { free(slots); }
	
	inline bool isEmpty() const
	{
		return count == 0;
	}
	
	inline bool active(const T* p) const
	{
		return p >= slots && p < slots + count;
	}
	
	inline T* begin()
	{
		return slots;
	}
	
	inline T* end()
	{
		return slots + count;
	}
	
	void clear()
	{
		if (slots) {
			while(count > 0) { release(&slots[count-1]); }
		}
	}

	template<typename... Args>
	T* alloc(Args&&... args)
	{
		if (count == capacity) {
			if (capacity == 0) {
				capacity = kDefaultReserve;
				slots = (T*) calloc(capacity, sizeof(T));
			} else {
				capacity += capacity;
				slots = (T*) realloc(slots, capacity * sizeof(T));
			}
		}
		++count;
		return new (&slots[count-1]) T(args...);
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
	void each(void (T0::*Func)(Args...), Args... args)
	{
		for(auto inst=begin(); inst!=end(); ++inst) {
			(inst->*Func)(args...);
		}
	}

	template<typename T0, typename ...Args>
	void cull(bool (T0::*Func)(Args...), Args... args)
	{
		for(auto inst=begin(); inst!=end();) {
			if ((inst->*Func)(args...)) { release(inst); } else { ++inst; }
		}
	}
	
};
