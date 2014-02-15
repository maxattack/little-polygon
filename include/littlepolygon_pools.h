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

	void reset() {
		mask.reset();
	}

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
		typename Bitset<N>::iterator biterator;

		iterator(const BitsetPool *pool) : slots((T*)pool->slots), biterator(&pool->mask) {
		}

	public:
		bool next(T* &result) {
			unsigned idx;
			if (biterator.next(idx)) {
				result = slots + idx;
				return true;
			} else {
				return false;
			}
		}
	};

	iterator listSlots() { return iterator(this); }
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
