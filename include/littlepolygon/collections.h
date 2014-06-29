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

//--------------------------------------------------------------------------------
// ARRAY THAT CHECKS BOUNDS IN DEBUG

template<typename T>
class Array {
private:
	#if DEBUG
	int n;
	#endif
	T* buf;
	
public:
	Array(int length)
	{
		#if DEBUG
		n = length;
		#endif
		buf = (T*) lpCalloc(length, sizeof(T));
		ASSERT(length == 0 || buf != 0);
	}
	
	Array(const Array<T>& noCopy);
	Array<T>& operator=(const Array<T>& noAssign);
	
	Array(Array<T>&& o) {
		#if DEBUG
		n = o.n;
		o.n = 0;
		#endif
		buf = o.buf;
		o.buf = 0;
	}
	
	~Array()
	{
		lpFree(buf);
	}
	
	T& operator[](int i)
	{
		ASSERT(i >= 0);
		ASSERT(i < n);
		return buf[i];
	}
	
	const T& operator[](int i) const
	{
		ASSERT(i >= 0);
		ASSERT(i < n);
		return buf[i];
	}
	
	T* operator+(int i)
	{
		ASSERT(i >= 0);
		ASSERT(i < n);
		return buf + i;
	}
	
	const T* operator+(int i) const
	{
		ASSERT(i >= 0);
		ASSERT(i < n);
		return buf + i;
	}
	
	T* ptr()
	{
		return buf;
	}
	
	const T* ptr() const
	{
		return buf;
	}
	
};

//--------------------------------------------------------------------------------
// SIMPLE TEMPLATE QUEUE (ring buffer)

template<typename T>
class Queue {
private:
	int cap, n, i;
	Array<T> slots;

public:
	Queue(int aCapacity) : cap(aCapacity), n(0), i(0), slots(cap) {
	}
	
	~Queue() {
		while(n > 0) {
			slots[i].~T();
			n--;
			i = (i+1) % cap;
		}
	}
	
	int capacity() const { return cap; }
	int count() const { return n; }
	bool empty() const { return n == 0; }
	bool full() const { return n == cap; }
	
	void enqueue(const T& val) {
		ASSERT(n < cap);
		new(&slots[(i + n) % cap]) T(val);
		n++;
	}
	
	template<typename... Args>
	void emplace(Args&&... args) {
		ASSERT(n < cap);
		new(&slots[(i + n) % cap]) T(std::forward<Args>(args) ...);
		n++;
	}
	
	T dequeue() {
		ASSERT(n > 0);
		T result = slots[i];
		n--;
		i = (i+1) % cap;
		return result;
	}

	T& peekNext() {
		ASSERT(n > 0);
		return slots[i];
	}
	
	T& peekLast() {
		ASSERT(n > 0);
		return slots[(i+n-1) % cap];
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
		
		bool next(T* outValue) {
			idx++;
			if (idx >= q->n) { return false; }
			*outValue = q->slots[(q->i + idx) % q->cap];
			return true;
		}
	};
};

//--------------------------------------------------------------------------------
// SIMPLE TEMPLATE LIST

template<typename T, bool grow=false>
class List {
private:
	static const int kDefaultCapacity = 32;
	unsigned cap, n;
	T* slots;

	void makeRoom() {
		if (slots == 0) {
			slots = (T*) lpCalloc(cap, sizeof(T));
			ASSERT(slots);
		} else if (n == cap) {
			ASSERT(grow);
			cap += cap;
			slots = (T*) lpRealloc(slots, cap * sizeof(T));
			ASSERT(slots);
		}
	}
	
public:
	List(unsigned aCapacity) : cap(aCapacity == 0 ? kDefaultCapacity : aCapacity), n(0), slots(0) {
	}
	
	List(const List<T, grow>& noCopy);
	List<T,grow>& operator=(const List<T,grow>& noAssign);

	List(List<T,grow>&& o) {		
		cap = o.cap;
		n = o.n;
		slots = o.slots;
		o.cap = 0;
		o.n = 0;
		o.slots = 0;
	}
	
	~List() {
		while(n > 0) {
			--n;
			slots[n].~T();
		}
		lpFree(slots);
	}
	
	int capacity() const { return cap; }
	int count() const { return n; }
	size_t rawCapacity() const { return cap * sizeof(T); }
	size_t rawSize() const { return n * sizeof(T); }
	bool empty() const { return n == 0; }
	bool full() const { return n == cap; }

	const T* begin() const { return slots; }
	const T* end() const { return slots + n; }
	T* begin() { return slots; }
	T* end() { return slots + n; }
	
	T get(int i) const {
		ASSERT(i >= 0);
		ASSERT(i < n);
		return slots[i];
	}

	T& operator[](int i) {
		ASSERT(i >= 0);
		ASSERT(i < n);
		return slots[i];
	}
	const T& operator[](int i) const {
		ASSERT(i >= 0);
		ASSERT(i < n);
		return slots[i];
	}
	
	void clear() {
		while(n > 0) {
			--n;
			slots[n].~T();
		}
	}
	
	void append(const T& val) {
		makeRoom();
		++n;
		new(&slots[n-1]) T(val);
	}
	
	template<typename... Args>
	T* alloc(Args&&... args) {
		makeRoom();
		++n;
		return new(slots + (n-1)) T(args ...);
	}
	
	T* move(T& val) {
		makeRoom();
		++n;
		return new(slots + (n-1)) T(std::move(val));
	}
	
	int offsetOf(const T* t) {
		ASSERT(t >= slots && t < slots + n);
		return t - slots;
	}
	
	bool tryAppend(const T& val) {
		if (n < cap) {
			append(val);
			return true;
		} else {
			return false;
		}
	}
	
	T& peekFirst() {
		ASSERT(n > 0);
		return slots[0];
	}
	
	T& peekLast() {
		ASSERT(n > 0);
		return slots[n-1];
	}
	
	T pop() {
		ASSERT(n > 0);
		n--;
		auto result = slots[n];
		slots[n].~T();
		return result;
	}
	
	void removeAt(int i) {
		ASSERT(i >= 0);
		ASSERT(i < n);
		for(int j=i+1; j<n; ++j) {
			slots[j-1] = slots[j];
		}
		n--;
		slots[n].~T();
	}
	
	void insertAt(const T& val, int i) {
		ASSERT(i >= 0);
		ASSERT(i <= n);
		ASSERT(n < cap);
		if (i == n) {
			append(val);
		} else {
			makeRoom();
			for(int j=n; j > i; --j) {
				slots[j] = slots[j-1];
			}
			slots[i] = val;
			n++;
		}
	}
	
	void fill(const T& val) {
		while(n < cap) { append(val); }
	}
	
	int findFirst(const T& val) const {
		for(int i=0; i<n; ++i) {
			if (slots[i] == val) { return i; }
		}
		return -1;
	}
	
	int findLast(const T& val) const {
		for(int i=n-1; i>=0; --i) {
			if (slots[i] == val) { return i; }
		}
		return -1;
	}

	bool contains(const T& val) const {
		return findFirst(val) != -1;
	}
	
	
};

//--------------------------------------------------------------------------------
// SIMPLE BITSET

class BitArray {
friend class BitLister;
private:
	unsigned capacity;
	Array<uint32_t> words;
	
public:
	BitArray(unsigned cap);
	
	void clear();
	void clear(unsigned i);
	void mark(unsigned i);

	bool operator[](unsigned i) const;
	
private:
	unsigned nwords() const { return (capacity + 31) >> 5; }
	static uint32_t bit(unsigned i);
	void getIndices(unsigned idx, unsigned* outWord, unsigned *outIndex) const;
};

class BitLister {
private:
	const BitArray *pArray;
	unsigned currentWord;
	unsigned currentIndex;
	uint32_t remainder;
	
public:
	BitLister(const BitArray *arr);
	bool next();
	
	unsigned index() const { ASSERT(currentIndex != -1); return (currentWord<<5) + currentIndex; }
};

//--------------------------------------------------------------------------------
// SIMPLE LINKED-LIST HEADER

struct Link {
	Link *prev, *next;
	
	Link() {
		next = this;
		prev = this;
	}
	
	~Link() {
		unbind();
	}
	
	void attachAfter(Link *before) {
		next = before->next;
		prev = before;
		before->next = this;
		next->prev = this;
	}
	
	void attachBefore(Link *after) {
		next = after;
		prev = after->prev;
		after->prev = this;
		prev->next = this;
	}
	
	void unbind() {
		next->prev = prev;
		prev->next = next;
		next = this;
		prev = this;
	}
	
	bool isBound() const {
		return next != this;
	}
};

template<typename T>
class Linkable : public Link, public T {
public:
	static T* getValue(Link *link) { return static_cast<Linkable<T>*>(link); }
	static Link* getLink(T* value) { return static_cast<Linkable<T>*>(value); }
};

