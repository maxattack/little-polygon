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
#include "bitset.h"

// Several different common/simple object pools that index into a linear
// array of preallocated slots.  Objects in the pool are not automatically
// finalized with the pool, so make sure to explicitly clear() them if the
// destructors matter.

#include "collections.h"
#include <algorithm>
#include <utility>
#include <vector>

//------------------------------------------------------------------------------
// BITSET POOL (Max 32 Elems)
//------------------------------------------------------------------------------

template<typename T, int N=32>
class BitsetPool {
public:
	typedef T InstanceType;

private:
	uint32_t mask;
	Slot<T> slots[N];
	
	
	inline static uint32_t bit(int i) {
		ASSERT(i >= 0);
		ASSERT(i < 32);
		return 0x80000000>>i;
	}
	
public:
	
	bool empty() const { return mask == 0; }
	bool full() const { return mask == (1<<N)-1; }
	
	int indexOf(T* p) const {
		ASSERT(active(p));
		return p - slots->address();
	}
	
	bool active(T* p) const {
		int i = p - slots->address();
		return (mask & bit(i)) != 0;
	}
	
	template<typename... Args>
	T* alloc(Args&&... args) {
		ASSERT(mask != 0xffffffff);
		auto i = __builtin_clz(~mask);
		mask |= bit(i);
		return new(&slots[i].record) T(args ...);
	}
	
	void release(T* p) {
		ASSERT(active(p));
		p->~T();
		int i = p - slots->address();
		mask ^= bit(i);
	}

	void clear() {
		while(mask) {
			auto i = __builtin_clz(mask);
			mask ^= bit(i);
			slots[i].release();
		}
		mask = 0;
	}
	
	class Subset {
	friend class BitsetPool<T>;
	private:
		uint32_t mask;
		
		Subset(uint32_t aMask) : mask(aMask) {}
		
	public:
		Subset() : mask(0) {}
		
		void add(int i) { mask |= bit(i); }
		void remove(int i) { mask &= ~bit(i); }
		void clear() { mask = 0; }
		void fill() { mask = 0xffffffff; }
		Subset intersect(Subset other) { return mask & other.mask; }
		void cull(BitsetPool<T> *p) { mask &= p->mask; }
	};
	
	

	class iterator {
	friend class BitsetPool<T>;
	private:
		T *slots;
		T *curr;
		uint32_t remainder;

		iterator(const BitsetPool<T> *pool) :
		slots((T*)pool->slots),
		remainder(pool->mask) {
		}
		
		iterator(const BitsetPool<T> *pool, Subset filter) :
		slots((T*)pool->slots),
		remainder(pool->mask & filter.mask) {
		}
		

	public:
		bool next() {
			if (remainder) {
				auto i = __builtin_clz(remainder);
				remainder ^= bit(i);
				curr = slots + i;
				return true;
			} else {
				curr = 0;
				return false;
			}
		}
		
		T* operator->() { return curr; }
		operator T*() { return curr; }
		
	};

	iterator list() { return iterator(this); }
	iterator list(Subset filter) { return iterator(this, filter); }


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
	CompactPool(size_t n) { slots.reserve(n); }
	
	// const methods
	inline const T* begin() const { return slots.data()->address(); }
	inline const T* end() const { return begin() + count(); }
	
	inline bool isEmpty() const { return slots.empty(); }
	inline int count() const { return slots.size(); }
	inline bool active(const T* p) const { return p >= begin() && p < end(); }
	inline int indexOf(const T* p) const { ASSERT(active(p)); return p - begin(); }

	// ordinary methods
	inline T* begin() { return slots.data()->address(); }
	inline T* end() { return begin() + count(); }
	
	inline void reserve(int n) { slots.reserve(n); }
	
	void clear() {
		for(auto& slot : slots) { slot.release(); }
		slots.clear();
	}

	template<typename... Args>
	T* alloc(Args&&... args) {
		slots.emplace_back();
		return slots.back().init(args...);
	}

	void release(T* p) {
		ASSERT(active(p));
		p->~T();
		if (p != slots.back().address()) {
			*p = slots.back().value();
		}
		slots.pop_back();
	}
	
	template<bool (T::*Func)()>
	void cull() {
		for(auto inst=begin(); inst!=end();) {
			if ((inst->*Func)()) { release(inst); } else { ++inst; }
		}
	}
	
};
