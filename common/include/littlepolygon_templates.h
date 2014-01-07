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
// LINEAR POOL TEMPLATE
// Combines a traditional pool and a compact array by separating slots and records
// into two arrays - the former for access and the latter for compactness.
//--------------------------------------------------------------------------------

#define MAX_POOL_CAPACITY (64*1024)
#define POOL_SLOT_INDEX(aID) (aID&0xffff)
typedef uint32_t ID;

struct LinearPoolSlot {
	ID id;
	uint16_t index;
	uint16_t next;
};

// This pool is endowed with a storage-fixed ID table 
// for looking up objects "by reference."  Objects must
// have a public "id" field
template<typename T, int N>
class LinearPool {
private:
	uint32_t mCount;
	T mRecords[N];
	LinearPoolSlot mSlots[N];
	uint16_t mFreelistEnqueue;
	uint16_t mFreelistDequeue;
	
public:
	LinearPool() : mCount(0), mFreelistEnqueue(N-1), mFreelistDequeue(0) {
		STATIC_ASSERT(N < MAX_POOL_CAPACITY);
		for(unsigned i=0; i<N; ++i) {
			mSlots[i].id = i;     // initialize lower-bits of id with index
			mSlots[i].next = i+1; // initialize the free slot linked-list
		}
	}

	bool isEmpty() const { return mCount == 0; }
	bool isFull() const { return mCount == N; }
	bool isActive(ID id) const { return mSlots[POOL_SLOT_INDEX(id)].id == id; }
	int count() const { return mCount; }
	T* begin() { return mRecords; }
	T* end() { return mRecords + mCount; }
	T& operator[](ID id) { ASSERT(isActive(id)); return mRecords[mSlots[POOL_SLOT_INDEX(id)].index]; }

	ID takeOut() {
		ASSERT(mCount < N);
		LinearPoolSlot &slot = mSlots[mFreelistDequeue]; // dequeue a new slot - we do this in FIFO order so that
		mFreelistDequeue = slot.next;                    // we don't "thrash" a record with interleaved add-remove calls
		                                                 // and use up the higher-order bits of the id
		slot.index = mCount++;                           // push a new record into the buffer
		mRecords[slot.index].id = slot.id;               // write the id to the record
		return slot.id;
    }

    void putBack(ID id) {
		ASSERT(isActive(id));
		
		LinearPoolSlot &slot = mSlots[POOL_SLOT_INDEX(id)];
		T& record = mRecords[slot.index];              // move the last record into this slot
		record = mRecords[--mCount];
		mSlots[POOL_SLOT_INDEX(record.id)].index = slot.index; // update the index from the moved record
        
		slot.id += 0x10000;                            // increment the fingerprint of the id
		
		if (mCount == N-1) {                           // enqueue the slot
			mFreelistEnqueue = POOL_SLOT_INDEX(id);
			mFreelistDequeue = POOL_SLOT_INDEX(id);
		} else {
			mSlots[mFreelistEnqueue].next = POOL_SLOT_INDEX(id);
			mFreelistEnqueue = POOL_SLOT_INDEX(id);
		}
    }
    
};

//--------------------------------------------------------------------------------
// BITSET WITH CLEVER ITERATION OF "1"s
//--------------------------------------------------------------------------------

template <unsigned tSize>
class Bitset {
	inline static unsigned clz(uint32_t word) { return __builtin_clz(word); }
	inline static uint32_t lz(unsigned bit) { return 0x80000000 >> bit; }
	inline static unsigned ones(uint32_t word) { return __builtin_popcount(word); }

	uint32_t nonzeroWords;
	uint32_t words[tSize/32];

public:

	Bitset() { 
		STATIC_ASSERT(tSize > 0);
		STATIC_ASSERT(tSize % 32 == 0);
		STATIC_ASSERT(tSize <= 1024);
		clear(); 
	}

	inline static unsigned size() { return tSize; }
	inline bool empty() const { return nonzeroWords == 0; }


	void mark(unsigned index) {
		ASSERT(index < tSize);
		unsigned word = index >> 5;
		unsigned bit = index & 31;
		nonzeroWords |= lz(word);
		words[word] |= lz(bit);
	}

	void clear(unsigned index) {
		ASSERT(index < tSize);
		unsigned word = index >> 5;
		unsigned bit = index & 31;
		words[word] &= ~lz(bit);
		if (words[word] == 0) {
			nonzeroWords &= ~lz(word);
		}
	}

	void mark() {
		const unsigned NUM_WORDS = tSize / 32;
		const unsigned NUM_ZEROES = 32 - NUM_WORDS;
		nonzeroWords = ~((1<<NUM_ZEROES)-1);
		memset(words, 0xff, sizeof(uint32_t) * NUM_WORDS);
	}

	void clear() {
		const unsigned NUM_WORDS = tSize / 32;
		nonzeroWords = 0;
		memset(words, 0, sizeof(uint32_t) * NUM_WORDS);
	}

	bool operator[](const unsigned index) const {
		ASSERT(index < tSize);
		unsigned word = index >> 5;
		unsigned bit = index & 31;
		return (words[word] & lz(bit)) != 0;
	}

	unsigned count() const {
		uint32_t remainder = nonzeroWords;
		unsigned c = 0;
		while(remainder != 0) {
			unsigned w = clz(remainder);
			remainder ^= lz(w);
			c += ones(words[w]);
		}
		return c;
	}

	bool findFirst(unsigned &index) const {
		if (nonzeroWords) {
			unsigned w = clz(nonzeroWords);
			index = clz(words[w]) + (w<<5);
			return true;
		} else {
			return false;
		}
	}

	bool clearFirst(unsigned &index) {
		if (nonzeroWords) {
			unsigned w = clz(nonzeroWords);
			index = clz(words[w]);
			words[w] ^= lz(index);
			if (words[w] == 0) {
				nonzeroWords ^= lz(w);
			}
			index += (w<<5);
			return true;
		} else {
			return false;
		}
	}

	class iterator {
	private:
		const Bitset<tSize> *bs;
		uint32_t remainder;
		unsigned w, v;

	public:
		iterator(const Bitset<tSize>& aSet) : bs(&aSet) {
			remainder = bs->nonzeroWords;
			if (remainder) {
				w = clz(remainder);
				v = bs->words[w];
			}
		}

		bool next(unsigned &index) {
			if (remainder) {
				index = clz(v);
				v ^= lz(index);
				index += (w<<5);
				if (v == 0) {
					remainder ^= lz(w);
					if (remainder) {
						w = clz(remainder);
						v = bs->words[w];
					}
				}
				return true;
			} else {
				return false;
			}
		}
	};

	iterator listBits() const { return iterator(*this); }

	Bitset<tSize>& operator &= (const Bitset<tSize> &other) {

		uint32_t remainder = nonzeroWords | other.nonzeroWords;
		while(remainder) {
			unsigned w = clz(remainder);
			remainder ^= lz(w);
			words[w] &= other.words[w];
			if (words[w] == 0) {
				nonzeroWords &= ~lz(w);
			}
		}

		return *this;
	}

	Bitset<tSize>& operator |= (const Bitset<tSize> &other) {
		nonzeroWords |= other.nonzeroWords;
		uint32_t remainder = nonzeroWords;
		while(remainder) {
			unsigned w = clz(remainder);
			remainder ^= lz(w);
			words[w] |= other.words[w];
		}
		return *this;
	}

	Bitset<tSize> operator ^= (const Bitset<tSize> &other) {
		int remainder = nonzeroWords | other.nonzeroWords;
		nonzeroWords = 0;
		while(remainder) {
			unsigned w = clz(remainder);
			remainder ^= lz(w);
			words[w] ^= other.words[w];
			if (words) {
				nonzeroWords |= lz(w);
			}
		}
		return *this;
	}

	Bitset<tSize> operator ~ () const {
		const unsigned NUM_WORDS = tSize/32;
		Bitset<tSize> result;
		for(int w=0; w<NUM_WORDS; ++w) {
			result.words[w] = ~words[w];
			if (result.words[w]) {
				result.nonzeroWords |= lz(w);
			}
		}
		return result;
	}
};

