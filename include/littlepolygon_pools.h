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
		reset();
	}

	void reset() {
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
	T slots[N];

public:

	void reset() {
		mask.reset();
	}

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
	T mSlots[N];

public:
	CompactPool() : mCount(0) {
	}

	void reset() {
		mCount = 0;
	}

	T* alloc() {
		ASSERT (mCount < N);
		++mCount;
		return mSlots + (mCount-1);
	}

	bool active(T* slot) const {
		return (slot - mSlots) >= 0 && (slot - mSlots) < mCount;
	}

	void release(T* slot) {
		ASSERT(active(slot));
		--mCount;
		if (slot != mSlots+mCount) {
			*slot = mSlots[mCount];
		}
	}

	void releaseValue(T value) {
		for(int i=0; i<mCount; ++i) {
			if (mSlots[i] == value) {
				release(mSlots + i);
				return;
			}
		}
	}

	int count() const { return mCount; }
	T* begin() { return mSlots; }
	T* end() { return mSlots + mCount; }

	bool isEmpty() const { return mCount == 0; }
	bool isFull() const { return mCount == N; }
};
