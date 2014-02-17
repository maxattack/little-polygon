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
// array of preallocated slots:
//
// * FreelistPool - Store unoccupied slots in a linked-list (no iteration)
// * BitsetPool - Store a bitvector which identifies unoccuped slots (allows iteration)
// * CompactPool - Use slots [0:count), swap-with last on release (fast iteration)
//                 (only useable for "anonymous" objects, like particles or bullets)
//

#include <utility>

//------------------------------------------------------------------------------
// FREE-LIST POOL
//------------------------------------------------------------------------------

template<typename T, int N>
class FreelistPool {
private:
	union Slot {
		T record;
		Slot *next;

		Slot() {}
		~Slot() {}
	};
	Slot *firstFree;
	Slot slots[N];

public:
	FreelistPool() {
		STATIC_ASSERT(N>1);
		reset();
	}

	void reset() {
		firstFree = slots;
		for(int i=0; i<N-1; ++i) {
			slots[i].next = slots + (i+1);
		}
		slots[N-1].next = 0;
	}

	template<typename... Args>
	T* alloc(Args&&... args) {
		if (!firstFree) {
			return 0;
		} else {
			auto result = firstFree;
			firstFree = result->next;
			return new(&(result->record)) T(std::forward<Args>(args) ...);
		}
	}

	void release(T* p) {
		Slot *slot = (Slot*)p;
		ASSERT(slot - slots >= 0);
		ASSERT(slot - slots < N);
		p->~T();
		slot->next = firstFree;
		firstFree = slot;
	}
};

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
		return new(&slots[index].record) T(std::forward<Args>(args) ...);
	}

	void release(T* p) {
		Slot *slot = (Slot*) p;
		ASSERT(slot - slots >= 0);
		ASSERT(slot - slots < N);
		p->~T();
		mask.clear(slot-slots);
	}

	class iterator {
	friend class BitsetPool<T,N>;
	private:
		T *slots;
		T *curr;
		typename Bitset<N>::iterator biterator;

		iterator(const BitsetPool *pool) : slots((T*)pool->slots), biterator(&pool->mask) {
		}

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

	iterator list() { return iterator(this); }
	
	void reset() {
		T* activeObject;
		for(auto i=list(); i.next();) {
			i->~T();
		}
		mask.reset();
	}

	
};

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
	
	int indexOf(T* p) {
		ASSERT(active(p));
		return ((Slot*)p) - slots;
	}
	
	bool active(T* p) {
		int i = ((Slot*)p) - slots;
		return (mask & bit(i)) != 0;
	}
	
	template<typename... Args>
	T* alloc(Args&&... args) {
		ASSERT(mask != 0xffffffff);
		auto i = __builtin_clz(~mask);
		mask |= bit(i);
		return new(&slots[i].record) T(std::forward<Args>(args) ...);
	}
	
	void release(T* p) {
		ASSERT(active(p));
		p->~T();
		int i = ((Slot*) p) - slots;
		mask ^= bit(i);
	}

	void reset() {
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
//------------------------------------------------------------------------------

template<typename T, int N>
class CompactPool {
private:
	int mCount;
	union Slot {
		T record;
		
		Slot() {}
		~Slot() {}
	};
	Slot mSlots[N];

public:
	CompactPool() : mCount(0) {
	}

	void reset() {
		for(int i=0; i<mCount; ++i) {
			mSlots[i].record.~T();
		}
		mCount = 0;
	}

	template<typename... Args>
	T* alloc(Args&&... args) {
		ASSERT (mCount < N);
		++mCount;
		return new(&mSlots[mCount-1].record) T(std::forward<Args>(args) ...);
	}

	bool active(T* p) const {
		auto slot = (Slot*)p;
		return (slot - mSlots) >= 0 && (slot - mSlots) < mCount;
	}

	void release(T* p) {
		ASSERT(active(p));
		auto slot = (Slot*) p;
		p->~T();
		--mCount;
		if (slot != mSlots+mCount) {
			// Errr... this bit right here is super-derpy.  I guess classes
			// should be using move semantics?  Like you "move" the object
			// into the "slot" and then finalize "mSlots+mCount"?  That's kinda
			// obnoxious, tho, since it means objects can be in this extra "hollowed-out"
			// state... :P
			memcpy(slot, mSlots+mCount, sizeof(Slot));
		}
	}

	int count() const { return mCount; }
	T* begin() { return (T*)mSlots; }
	T* end() { return ((T*)mSlots) + mCount; }

	bool isEmpty() const { return mCount == 0; }
	bool isFull() const { return mCount == N; }
};
