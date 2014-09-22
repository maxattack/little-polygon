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

#include "collections.h"

//------------------------------------------------------------------------------
// GENERIC COMPONENT POOL

template<typename T>
class Pool
{
private:
	Array<T> mRecords;
	Array<T*> mRoster;
	Array<T**> mBackRefs;
	T** mCurr;
	T** mEnd;
	int mCount;
	int mCap;


public:
	Pool(int n=1024)
	: mRecords(n), mRoster(n), mBackRefs(n), mCurr(0), mEnd(0), mCount(0), mCap(n)
	{
		for(int i=0; i<n; ++i) {
			mRoster[i] = mRecords + i;
		}
	}
	
	~Pool()
	{
		clear();
	}

	int count() const { return mCount; }
	int cap() const { return mCap; }
	bool isActive(T* inst) const { return mBackRefs[getIndex(inst)] != 0; }

	template<typename... Args>
	T* alloc(Args&&... args)
	{
		ASSERT(mCount  < mCap);
		auto inst = mRoster[mCount];
		mBackRefs[getIndex(inst)] = mRoster + mCount;
		++mCount;
		return new(inst) T (std::forward<Args>(args)...);
	}
	
	void clear()
	{
		ASSERT(mCurr == 0);
		for(auto p=mRoster.ptr(), end=p+mCount; p!=end; ++p) {
			(*p)->~T();
		}
		mCount = 0;
	}
	
	void release(T* inst)
	{
		ASSERT(isActive(inst));
		auto iter = getIter(inst);
		--mCount;
		if (iter >= mEnd) {
			// element is after the iteration slice, 1 swap
			doSwap(inst, mRoster[mCount]);
		} else if (iter >= mCurr) {
			// element is inside the iteration slice, 2 swaps
			--mEnd;
			doSwap(inst, *mEnd);
			doSwap(*mEnd, mRoster[mCount]);
		} else {
			// element is before the iteration slice, 3 swaps
			--mCurr;
			--mEnd;
			doSwap(inst, *mCurr);
			doSwap(*mCurr, *mEnd);
			doSwap(*mEnd, mRoster[mCount]);
		}		
		mBackRefs[getIndex(inst)] = 0;
		inst->~T();
	}

	void iterBegin()
	{
		mCurr = mRoster.ptr();
		mEnd = mRoster.ptr() + mCount;
	}

	void iterCancel()
	{
		mCurr = 0;
		mEnd = 0;
	}

	T* iterNext()
	{
		if (mCurr != mEnd) {
			auto result = *mCurr;
			++mCurr;
			return result;
		}  else {
			mCurr = 0;
			mEnd = 0;
			return 0;
		}
	}

private:
	inline int getIndex(T* inst) const { return (int)(inst - mRecords.ptr()); }
	inline T** getIter(T* inst) const { return mBackRefs[getIndex(inst)]; }

	void doSwap(T* slot, T* tail)
	{
		if (slot != tail) {
			auto slotIdx = getIndex(slot);
			auto tailIdx = getIndex(tail);
			std::swap(*mBackRefs[slotIdx], *mBackRefs[tailIdx]);
			std::swap(mBackRefs[slotIdx], mBackRefs[tailIdx]);
		}
	}

};

//------------------------------------------------------------------------------
// COMPACT POOL
// Faster iteration and no upper-bound on size, but records are not guaranteed
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

//--------------------------------------------------------------------------------
// BATCHES are extend Compact Pools with an index so that callers can received a
// two-star "handle" to a logical record in the pool, even though it still moves
// around.
//
// Batches are optimized for component systems which benefit from memory locality
// of records and won't to modify the collection during iteration.  A good example
// is a batch of draw calls.
//
// WARNING: Records in batches will be treated like Plain-Old-Data.  Objects will
// be moved using memcpy() and released using free().  As a convenience, they can
// have nontrivial constructors, but no destructors.
//--------------------------------------------------------------------------------

template<typename T>
union BatchIndex 
{
	T* slot;
	BatchIndex<T>* next;
};

template<typename T>
struct BatchHandle
{
	BatchIndex<T>* index;

	BatchHandle() {}
	BatchHandle(BatchIndex<T>* aHandle) : index(aHandle) {}

	T& operator*() { return *(index->slot); }
	T* operator->() { return index->slot; }
	const T& operator*() const { return *(index->slot); }
	const T* operator->() const { return index->slot; }
	
	bool operator==(const BatchHandle<T>& h) const { return index == h.index; }
	bool operator!=(const BatchHandle<T>& h) const { return index != h.index; }
};

template<typename T>
class BatchPool
{
private:
	int mCount, mCap;
	Array<T> mSlots;
	Array<BatchIndex<T>> mIndex;
	Array<BatchIndex<T>*> mBack;
	BatchIndex<T>* mFreelist;

public:

	BatchPool(int cap=1024) : mCount(0), mCap(cap), mSlots(cap), mIndex(cap), mBack(cap)
	{
		resetFreelist();
	}

	bool isEmpty() const { return mCount == 0; }
	bool isFull() const { return mCount == mCap; }
	bool isActive(BatchHandle<T> h) const { return h.index->slot >= begin() && h.index->slot < end(); }

	T* begin() { return mSlots.ptr(); }
	T* end() { return mSlots.ptr()+mCount; }
	
	const T* begin() const { return mSlots.ptr(); }
	const T* end() const { return mSlots.ptr()+mCount; }

	template<typename... Args> 
	BatchHandle<T> alloc(Args&&... args)
	{
		ASSERT(mFreelist);
		
		// pop a handle from the freelist
		auto result = mFreelist;
		mFreelist = mFreelist->next;

		// append a new slot to the end
		auto idx = mCount;
		++mCount;
		result->slot = new(mSlots+idx) T (std::forward<Args>(args)...);
		mBack[idx] = result;
		return result;
	}

	void release(BatchHandle<T> handle)
	{
		ASSERT(isActive(handle));
		--mCount;
		
		auto i = (int) (handle.index->slot - mSlots.ptr());
		if (i != mCount) {
			// fill "hole" with last element
			memcpy(mSlots + i, mSlots + mCount, sizeof(T));
			memcpy(mBack + i, mBack + mCount, sizeof(BatchIndex<T>*));

			// update handle to reflect moved record 
			mBack[i]->slot = mSlots + i;
		}

		// return handle to freelist
		handle.index->next = mFreelist;
		mFreelist = handle.index;
	}

	void clear()
	{
		mCount = 0;
		resetFreelist();
	}

private:

	void resetFreelist()
	{
		mFreelist = mIndex.ptr();
		for(int i=0; i<mCap-1; ++i) {
			mIndex[i].next = mIndex + (i+1);
		}
		mIndex[mCap-1].next = 0;				
	}

};
