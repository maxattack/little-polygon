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
	};
	Slot *firstFree;
	Slot slots[N];

public:
	FreelistPool() {
		STATIC_ASSERT(N>1);
		firstFree = slots;
		for(int i=0; i<N-1; ++i) {
			slots[i].next = slots + (i+1);
		}
		slots[N-1].next = 0;
	}

	T* alloc() {
		if (!firstFree) {
			return 0;
		} else {
			auto result = firstFree;
			firstFree = result->next;
			return &(result->record);
		}			
	}

	void release(T* slot) {
		Slot *p = (Slot*)slot;
		ASSERT(p - slots >= 0);
		ASSERT(p - slots < N);		
		p->next = firstFree;
		firstFree = p;
	}
};

//------------------------------------------------------------------------------
// BITSET POOL
//------------------------------------------------------------------------------

template<typename T, int N>
class BitsetPool {
private:
	Bitset<N> mask;
	size_t capacity;
	T slots[N];

public:

	T* alloc() {
		unsigned index;
		if (!(~mask).findFirst(index)) {
			return 0;
		}
		mask.mark(index);
		return slots + index;
	}

	void release(T* slot) {
		ASSERT(slot - slots >= 0);
		ASSERT(slot - slots < N);
		mask.clear(slot-slots);
	}

	class iterator {
	friend class BitsetPool<T,N>;
	private:
		T *slots;
		typename Bitset<N>::iterator biterator;

		iterator(const BitsetPool *pool) : slots((T*)pool->slots), biterator(pool->mask) {
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
	T slots[N];

public:
	CompactPool() : mCount(0) {}

	T* alloc() {
		if (mCount >= N) {
			return 0;
		} else {
			++mCount;
			return slots + (mCount-1);
		}
	}

	void release(T* slot) {
		ASSERT(slot - slots >= 0);
		ASSERT(slot - slots < mCount);
		--mCount;
		if (slot != slots+mCount) {
			*slot = slots[mCount];
		}
	}

	void releaseValue(T value) {
		for(int i=0; i<mCount; ++i) {
			if (slots[i] == value) {
				release(slots + i);
				return;
			}
		}
	}

	int count() const { return mCount; }
	T* begin() { return slots; }
	T* end() { return slots + mCount; }

	bool isFull() const { return mCount == N; }
};


