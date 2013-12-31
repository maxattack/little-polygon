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
#include "littlepolygon.h"

//--------------------------------------------------------------------------------
// SIMPLE TEMPLATE QUEUE
//--------------------------------------------------------------------------------

template<typename T, int N>
class Queue {
private:
	int n;
	int i;
	T buf[N];

public:
	Queue() : n(0), i(0) {}
	
	int capacity() const { return N; }
	int count() const { return n; }
	bool empty() const { return n == 0; }
	bool full() const { return n == N; }
	
	void enqueue(const T& val) {
		ASSERT(n < N);
		buf[(i + n) % N] = val;
		n++;
	}
	
	T dequeue() {
		ASSERT(n > 0);
		T result = buf[i];
		n--;
		i = (i+1) % N;
		return result;
	}

	T peekNext() {
		ASSERT(n > 0);
		return buf[i];
	}
	
	T peekLast() {
		ASSERT(n > 0);
		return buf[(i+n-1) % N];
	}
	
	bool tryDequeue(T* outValue) {
		if (n > 0) {
			*outValue = dequeue();
			return true;
		} else {
			return false;
		}
	}
	
	class Iterator {
	private:
		Queue *q;
		int idx;
		
	public:
		Iterator(Queue& queue) : q(&queue), idx(-1) {
		}
		
		bool step(T* outValue) {
			idx++;
			if (idx >= q->n) { return false; }
			*outValue = q->buf[(q->i + idx) % N];
			return true;
		}
	};
};

//--------------------------------------------------------------------------------
// SIMPLE TEMPLATE LIST/STACK
//--------------------------------------------------------------------------------

template<typename T, int N>
class List {
private:
	int n;
	T buf[N];

public:
	List() : n(0) {}
	
	int capacity() const { return N; }
	int count() const { return n; }
	size_t rawCapacity() const { return N * sizeof(T); }
	size_t rawSize() const { return n * sizeof(T); }
	bool empty() const { return n == 0; }
	bool full() const { return n == N; }

	T* begin() const { return buf; }
	T* end() const { return buf+n; }
	
	T get(int i) const {
		ASSERT(i >= 0);
		ASSERT(i < n);
		return buf[i];
	}

	T& operator[](int i) {
		ASSERT(i >= 0);
		ASSERT(i < n);
		return buf[i];
	}
	
	void clear() {
		n = 0;
	}
	
	void append(const T& val) {
		ASSERT(n < N);
		buf[n] = val;
		n++;
	}
	
	T& peekFirst() {
		ASSERT(n > 0);
		return buf[0];
	}
	
	T& peekLast() {
		ASSERT(n > 0);
		return buf[n-1];
	}
	
	T& push() {
		ASSERT(n < N);
		n++;
		return buf[n-1];
	}
	
	T pop() {
		ASSERT(n > 0);
		n--;
		return buf[n];
	}
	
	void removeAt(int i) {
		ASSERT(i >= 0);
		ASSERT(i < n);
		for(int j=i+1; j<n; ++j) {
			buf[j-1] = buf[j];
		}
		n--;
	}
	
	void insertAt(const T& val, int i) {
		ASSERT(i >= 0);
		ASSERT(i <= n);
		ASSERT(n < N);
		if (i == n) {
			push(val);
		} else {
			for(int j=n; j > i; --j) {
				buf[j] = buf[j-1];
			}
			buf[i] = val;
			n++;
		}
	}
	
	int indexOf(const T* val) const {
		ASSERT(val >= buf);
		ASSERT(val < buf + n);
		return int(val - buf);
	}
	
	int find(const T& val) const {
		for(int i=0; i<n; ++i) {
			if (buf[i] == val) { return i; }
		}
		return -1;
	}
	
	bool contains(const T& val) const {
		return find(val) != -1;
	}
};

//--------------------------------------------------------------------------------
// SIMPLE OBJECT POOL TEMPLATES
// Both pools store all the active objects in a compact array for fast 
// batch-processing.  When items are removed it's slot is exchanged with 
// the last item.
//--------------------------------------------------------------------------------

#define MAX_POOL_CAPACITY (64*1024)

typedef uint32_t ID;

struct PoolSlot {
    ID id;
    uint16_t index;
    uint16_t next;
};

struct Poolable {
	ID id;
};

// This pool is endowed with a storage-fixed ID table 
// for looking up objects "by reference."  Objects must
// have a public "id" field (easily accomplished by subclassing
// Poolable above).
template<typename T, int N>
class Pool {
private:
    uint32_t mCount;
    T mRecords[N];
    PoolSlot mSlots[N];
    uint16_t mFreelistEnqueue;
    uint16_t mFreelistDequeue;
	
public:
    Pool() : mCount(0), mFreelistEnqueue(N-1), mFreelistDequeue(0) {
        STATIC_ASSERT(N < MAX_POOL_CAPACITY);
        // initialize the free queue linked-list
        for(unsigned i=0; i<N; ++i) {
            mSlots[i].id = i;
            mSlots[i].next = i+1;
        }
    }
	
	bool isEmpty() const {
		return mCount == 0;
	}
	
	bool isFull() const {
		return mCount == N;
	}
	
    bool isActive(ID id) const {
        // use the lower-bits to find the record
        return mSlots[id & 0xffff].id == id;
    }
	
    T& operator[](ID id) {
        ASSERT(isActive(id)); 
        return mRecords[mSlots[id & 0xffff].index]; 
    }

    ID takeOut() {
        ASSERT(mCount < N);
        // dequeue a new index record - we do this in FIFO order so that
        // we don't "thrash" a record with interleaved add-remove calls
        // and use up the higher-order bits of the id
        PoolSlot &slot = mSlots[mFreelistDequeue];
		mFreelistDequeue = slot.next;
        // push a new record into the buffer
        slot.index = mCount++;
        // write the id to the record
        mRecords[slot.index].id = slot.id;
		return slot.id;
    }

    void putBack(ID id) {
        // assuming IDs are valid in production
        ASSERT(isActive(id));
        // lookup the index record
        PoolSlot &slot = mSlots[id & 0xffff];
		// move the last record into this slot
		T& record = mRecords[slot.index];
		record = mRecords[--mCount];
		// update the index from the moved record
		mSlots[record.id & 0xffff].index = slot.index;
        // increment the higher-order bits of the id (a fingerprint)
        slot.id += 0x10000;
		if (mCount == N-1) {
			mFreelistEnqueue = id & 0xffff;
			mFreelistDequeue = id & 0xffff;
		} else {
			mSlots[mFreelistEnqueue].next = id & 0xffff;
			mFreelistEnqueue = id & 0xffff;
		}
    }
    
	int count() const { return mCount; }

    uint16_t indexOf(ID id) { 
        return mSlots[id & 0xffff].index; 
    }

    T* begin() { 
        return mRecords; 
    }

    T* end() { 
        return mRecords + mCount; 
    }
};

// The anonymous pool is lighter weight than the regular pool because
// objects are not identified uniquely, but treated as a group.  Pointers
// returns from takeOut() are only valid until another non-const method on the pool
// is called.
template<typename T, int N>
class AnonymousPool {
private:
    uint32_t mCount;
    T mRecords[N];
	
public:
	AnonymousPool() : mCount(0) {
	}
	
	int count() const { return mCount; }
	bool isFull() const { return mCount == N; }
	T* begin() { return mRecords; }
	T* end() { return mRecords+mCount; }
	
	T* takeOut() {
		ASSERT(mCount < N);
		mCount++;
		return mRecords + (mCount-1);
	}
	
	void putBack(T* t) {
		int n = t - mRecords;
		ASSERT(n < mCount);
		mCount--;
		mRecords[n] = mRecords[mCount];
	}
	
	void drain() {
		mCount = 0;
	}
};


