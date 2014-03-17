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
public:
	typedef T InstanceType;
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

template<typename T, int N=32>
class BitsetPool32 {
public:
	typedef T InstanceType;

private:
	uint32_t mask;
	union Slot {
		T record;
		Slot() {}
		~Slot() {}
	};
	Slot slots[N];
	
	
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
// DYNAMIC POOL
// Starts with 32 elements and then doubles-on-overflow up to capacity, for when
// you don't want to statically preallocate all the slots on init for all your
// pools.
//------------------------------------------------------------------------------

template<typename T, int N=1024>
class DynamicPool {
public:
	typedef T InstanceType;
	
private:
	friend class iterator;
	union Slot {
		T record;
		Slot() {}
		~Slot() {}
	};
	typedef Slot* SlotPtr;
	
	// Similar to a bitset pool, but we only allocate 32 instances
	// in-place.  Beyond that we lazily malloc additiona
	// instances, doubling capacity every time we overflow.
	
	Bitset<N> mask;
	Slot baseSlots[32];
	SlotPtr extendedSlots[5];
	
public:
	DynamicPool() {
		memset(extendedSlots, 0, 5 * sizeof(SlotPtr));
	}
	
	~DynamicPool() {
		// free memory
		for(int i=0; i<5 && extendedSlots[i]; ++i) {
			LITTLE_POLYGON_FREE(extendedSlots[i]);
		}
	}
	
	inline bool empty() const { return mask.empty(); }
	inline bool active(T *s) const { return mask[getIndex((Slot*)s)]; }
	
	template<typename... Args>
	T* alloc(Args&&... args) {
		
		// find the lowest zero in the mask
		unsigned index;
		if (!(~mask).findFirst(index)) {
			return 0;
		}
		
		// allocate a slot
		Slot *slot;
		if (index < 32) {
			slot = baseSlots + index;
		} else {
			auto buf = indexToBuffer(index);
			if (!extendedSlots[buf]) {
				allocateBuffer(buf);
			}
			slot = extendedSlots[buf] + localIndex(index, buf);
		}
		
		// placement new
		auto result = new(slot) T(args ...);
		
		// we mark the slot after constructing the object, so that
		// they can check if they're "first", and trigger side-effects
		// in that case.
		mask.mark(index);
		
		return result;
		
	}
	
	void release(T* p) {
		
		// clear from mask
		int i = getIndex((Slot*)p);
		ASSERT(mask[i]);
		mask.clear(i);
		
		// logically release
		p->~T();
		
	}
	
	// iteration
	
	class iterator {
		friend class DynamicPool<T>;
	private:
		DynamicPool<T> *pool;
		T *curr;
		typename Bitset<1024>::iterator biterator;
		
		iterator(DynamicPool *apool) :
		pool(apool), biterator(&pool->mask) {}
		
	public:
		bool next() {
			unsigned idx;
			if (biterator.next(idx)) {
				curr = (T*) pool->getSlot(idx);
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
	
private:
	
	inline Slot* getSlot(int i) {
		ASSERT(i >= 0);
		// check if we're preallocated or dynamically allocated
		if (i < 32) {
			return baseSlots + i;
		} else {
			int buf = indexToBuffer(i);
			return extendedSlots[buf] + localIndex(i, buf);
		}
	}
	
	inline int indexToBuffer(int i) const {
		ASSERT(i >= 32);
		
		// previous-power-of-two since lengths double
		return (31 - __builtin_clz(i))-5;
	}
	
	inline int lengthOfBuffer(int buf) const {
		ASSERT(buf >= 0);
		ASSERT(buf < 5);
		
		// Each buffers doubles the size of the pool
		return 32 << buf;
	}
	
	inline int firstIndex(int buf) const {
		// Because doubling, first is length
		return lengthOfBuffer(buf);
	}
	
	inline int localIndex(int i, int buf) const {
		ASSERT(buf >= 0);
		ASSERT(buf < 5);
		ASSERT(i >= 0);
		ASSERT(i < 1024);
		return i - firstIndex(buf);
	}
	
	inline void allocateBuffer(int i) {
		ASSERT(i >= 0 && i < 5);
		ASSERT(extendedSlots[i] == 0);
		extendedSlots[i] = (Slot*) LITTLE_POLYGON_MALLOC(sizeof(Slot) * lengthOfBuffer(i));
	}
	
	inline int getIndex(Slot* p) const {
		// because we don't know where malloc put things, we need to do a dumb
		// lookup here.
		if (p >= baseSlots && p < baseSlots + 32) {
			return p - baseSlots;
		}
		
		for(int i=0; i<arraysize(extendedSlots) && extendedSlots[i]; ++i) {
			auto len = lengthOfBuffer(i);
			if (p >= extendedSlots[i] && p < extendedSlots[i] + len) {
				return len + (p - extendedSlots[i]);
			}
		}
		// we're assuming here that we found the instance
		ASSERT(false);
		return -1;
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
	inline T* begin() const { return (T*) slots.data(); }
	inline T* end() const { return begin() + count(); }
	
	inline bool isEmpty() const { return slots.empty(); }
	inline int count() const { return slots.size(); }
	inline bool active(const T* p) const { return p >= begin() && p < end(); }
	inline int indexOf(const T* p) const { ASSERT(active(p)); return p - slots.data(); }

	// ordinary methods
	inline T* begin() { return (T*) slots.data(); }
	inline T* end() { return begin() + count(); }
	
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
		if (slot != &slots.back()) {
			*slot = slots.back();
		}
		slots.pop_back();
	}

};

//------------------------------------------------------------------------------
// INDEXED COMPACT POOL
// Combined a compact pool with an "index" that tracks the location of logical
// records from a fixed memory position.  This is a kind of "best of both worlds"
// approach where we get compact linear iteration, but also a fixed memory handle.
// TODO: Flexible capacity?
//------------------------------------------------------------------------------

template<typename T>
class Handle {
private:
	T** handle;

public:
	inline Handle() : handle(0) {}
	inline Handle(T** aHandle) : handle(aHandle) {}
	inline operator T*() { return *handle; }
	inline T* operator->() { return *handle; }
	inline T* operator->() const { return *handle; }
	inline T& operator*() { return **handle; }
};

template<typename T>
class IndexedCompactPool {
public:
	typedef T* InstanceType;

private:
	union Slot {
		T record;
		Slot() {}
		~Slot() {}
	};
	
	union Index {
		Slot* slot;
		Index *next;
	};
	
	typedef Index* IndexPtr;

	int n, capacity;
	
	Index *indices;
	Slot *slots;
	IndexPtr *backIndices;
	Index *nextFreeIndex;
	
public:
	IndexedCompactPool(int cap) : n(0), capacity(cap) {
		auto backingStore = LITTLE_POLYGON_MALLOC(
			capacity * (sizeof(Slot) + sizeof(Index) + sizeof(IndexPtr))
		);
		indices = (Index*)(backingStore);
		slots = (Slot*)(indices + capacity);
		backingStore = (IndexPtr*)(slots + capacity);
		
		nextFreeIndex = indices;
		for(int i=0; i<capacity-1; ++i) {
			indices[i].next = indices + i + 1;
		}
		indices[capacity-1].next = 0;
	}
	
	~IndexedCompactPool() {
		LITTLE_POLYGON_FREE(indices);
	}
	
	bool empty() { return n == 0; }
	inline int count() { return count; }
	
	template<typename ...Args>
	T** alloc(Args&&... args) {
		ASSERT(n < capacity);
		
		// pop a new index
		auto pIndex = nextFreeIndex;
		nextFreeIndex = pIndex->next;
		
		// init the last slot position and back reference
		pIndex->slot = slots + n;
		new (pIndex->slot) T(args...);
		backIndices[n] = pIndex;
		++n;
		
		return (T**) pIndex;
	}
	
	void release(T** handle) {
		ASSERT(n > 0);
		auto pIndex = (Index*) handle;
		
		// release slots
		pIndex->slot.record.~T();
		--n;
		
		if (pIndex->slot < (slots + n)) {
			// switch with last slot
			Index* lastIndex = backIndices[n];
			backIndices[pIndex->slot - slots] = lastIndex;
			*pIndex->slot = *lastIndex->slot;
			lastIndex->slot = pIndex->slot;
		}

		// return index to free pool
		pIndex->next = nextFreeIndex;
		nextFreeIndex = pIndex;
	}
	
	// Simple Linear iteration
	inline T* begin() { return slots; }
	inline T* end() { return slots + n; }
	inline T& operator[](int i) { ASSERT(i >= 0 && i < n); return slots[i].record; }
	
	void clear() {
		for(int i=0; i<n; ++i) {
			slots[i].record.~T();
			backIndices[i].next = nextFreeIndex;
			nextFreeIndex = backIndices[i];
		}
		n = 0;
	}
	
};

