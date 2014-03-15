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

#include "littlepolygon_base.h"
#include "littlepolygon_bitset.h"

// Several different common/simple object pools that index into a linear
// array of preallocated slots.  Objects in the pool are not automatically
// finalized with the pool, so make sure to explicitly clear() them if the
// destructors matter.

#include <algorithm>
#include <utility>
#include <vector>

//------------------------------------------------------------------------------
// BITSET POOL
//------------------------------------------------------------------------------

template<typename T, int N>
class BitsetPool {
private:
	Bitset<N> mask;
	union Slot {
		T record;
		Slot() {}
		~Slot() {}
	};
	Slot slots[N];

public:
	
	template<typename... Args>
	T* alloc(Args&&... args) {
		unsigned index;
		if (!(~mask).findFirst(index)) {
			return 0;
		}
		mask.mark(index);
		return new(&slots[index].record) T(args ...);
	}

	void release(T* p) {
		ASSERT(contains(p));
		Slot *slot = (Slot*) p;
		ASSERT(mask[slot-slots]);
		p->~T();
		mask.clear(slot-slots);
	}

	class iterator {
	friend class BitsetPool<T,N>;
	private:
		T *slots;
		T *curr;
		typename Bitset<N>::iterator biterator;

		iterator(const BitsetPool *pool) :
			slots((T*)pool->slots),
			biterator(&pool->mask) {}

	public:
		bool next() {
			unsigned idx;
			if (biterator.next(idx)) {
				curr = slots + idx;
				return true;
			} else {
				curr = 0;
				return false;
			}
		}
		
		T* operator->() { return curr; }
		operator T*() { return curr; }
		
	};
	
	bool contains(void *p) { return p >= slots && p < slots + N; }
	
	T& operator[](int i) {
		ASSERT(mask[i]);
		return slots[i].record;
	}
	
	bool active(T* record) const {
		int index = record - (T*)slots;
		ASSERT(index >= 0 && index < N);
		return mask[index];
	}
	
	int indexOf(T* record) const {
		ASSERT(active(record));
		return record - (T*)slots;
	}
	
	bool full() const { return mask.full(); }

	iterator list() { return iterator(this); }
	iterator list(Bitset<N> subset) { return iterator(this, subset); }
	
	void clear() {
		for(auto i=list(); i.next();) { i->~T(); }
		mask.reset();
	}
	
};

//------------------------------------------------------------------------------
// BITSET POOL (noodled for 32 elements, because 32-bit, amiright?)
//------------------------------------------------------------------------------

template<typename T>
class BitsetPool32 {
private:
	uint32_t mask;
	union Slot {
		T record;
		Slot() {}
		~Slot() {}
	};
	Slot slots[32];
	
	
	inline static uint32_t bit(int i) {
		ASSERT(i >= 0);
		ASSERT(i < 32);
		return 0x80000000>>i;
	}
	
public:
	
	bool empty() const { return mask == 0; }
	bool full() const { return mask == 0xffffffff; }
	
	int indexOf(T* p) const {
		ASSERT(active(p));
		return ((Slot*)p) - slots;
	}
	
	bool active(T* p) const {
		int i = ((Slot*)p) - slots;
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
		int i = ((Slot*) p) - slots;
		mask ^= bit(i);
	}

	void clear() {
		while(mask) {
			auto i = __builtin_clz(mask);
			mask ^= bit(i);
			slots[i].record.~T();
		}
		mask = 0;
	}
	
	class Subset {
	friend class BitsetPool32<T>;
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
		void cull(BitsetPool32<T> *p) { mask &= p->mask; }
	};
	
	

	class iterator {
	friend class BitsetPool32<T>;
	private:
		T *slots;
		T *curr;
		uint32_t remainder;

		iterator(const BitsetPool32<T> *pool) :
		slots((T*)pool->slots),
		remainder(pool->mask) {
		}
		
		iterator(const BitsetPool32<T> *pool, Subset filter) :
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
private:
	union Slot {
		T record;
		Slot() {}
		~Slot() {}
	};
	std::vector<Slot> slots;

public:
	
	CompactPool() {}
	CompactPool(size_t n) : slots(n) {}
	
	// const methods
	
	inline const T* begin() const { return slots.begin(); }
	inline const T* end() const { return slots.end(); }

	inline bool isEmpty() const { return slots.empty(); }
	inline int count() const { return slots.size(); }
	inline bool active(const T* p) const { return p >= begin() && p < end(); }
	inline int indexOf(const T* p) const { ASSERT(active(p)); return p - begin(); }

	// ordinary methods
	
	inline T* begin() { return slots.begin(); }
	inline T* end() { return slots.end(); }
	
	inline void reserve(int n) { slots.reserve(n); }
	inline T& operator[](int i) { ASSERT(i >= 0 && i < count()); return slots[i].record; }
	
	void clear() {
		for(auto& slot : slots) { slot.record.~T(); }
		slots.clear();
	}

	template<typename... Args>
	T* alloc(Args&&... args) {
		slots.emplace_back(args ...);
		return &(slots.back().record);
	}

	void release(T* p) {
		ASSERT(active(p));
		auto slot = (Slot*) p;
		p->~T();
		if (p != slots.back()) {
			*p = slots.back();
		}
		slots.pop_back();
	}

};


