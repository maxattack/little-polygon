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
#include <utility>
#include <type_traits>

//--------------------------------------------------------------------------------
// POD "Storage Slot" For a value (unrestricted-union workaround :P)
// The basic idea is we'd like to reserve space for an object, but explicitly
// call its constructor and destructor.
//--------------------------------------------------------------------------------

template<typename T>
struct Slot {
	// This is better than a char[] because it handles alignment
	// correctly and portably ;)
    typename std::aligned_storage<sizeof(T), __alignof(T)>::type storage;

    Slot() {}
    ~Slot() {}

    template<typename... Args>
    T* init(Args&&... args) { return new(address()) T(std::forward<Args>(args) ...); }
    void release() { (*address()).~T(); }

    const T* address() const { return static_cast<const T*>(static_cast<const void*>(&storage)); }
	T value() const { return *address(); }

	T* address() { return static_cast<T*>(static_cast<void*>(&storage)); }
	T& reference() { return *address(); }
};

//--------------------------------------------------------------------------------
// SIMPLE TEMPLATE QUEUE
//--------------------------------------------------------------------------------

template<typename T, int N>
class Queue {
private:
	int n;
	int i;
	Slot<T> slots[N];

public:
	Queue() : n(0), i(0) {}
	
	int capacity() const { return N; }
	int count() const { return n; }
	bool empty() const { return n == 0; }
	bool full() const { return n == N; }
	
	void enqueue(const T& val) {
		ASSERT(n < N);
		new(slots[(i + n) % N].address()) T(val);		
		n++;
	}
	
	template<typename... Args>
	void emplace(Args&&... args) {
		ASSERT(n < N);
		new(slots[(i + n) % N].address()) T(std::forward<Args>(args) ...);
		n++;
	}
	
	T dequeue() {
		ASSERT(n > 0);
		T result = slots[i].value();
		n--;
		i = (i+1) % N;
		return result;
	}

	T& peekNext() {
		ASSERT(n > 0);
		return slots[i].reference();
	}
	
	T& peekLast() {
		ASSERT(n > 0);
		return slots[(i+n-1) % N].reference();
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
			*outValue = q->slots[(q->i + idx) % N].value();
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
	Slot<T> slots[N];

public:
	List() : n(0) {}
	
	int capacity() const { return N; }
	int count() const { return n; }
	size_t rawCapacity() const { return N * sizeof(T); }
	size_t rawSize() const { return n * sizeof(T); }
	bool empty() const { return n == 0; }
	bool full() const { return n == N; }

	T* begin() const { return slots->address(); }
	T* end() const { return begin() + n; }
	
	T get(int i) const {
		ASSERT(i >= 0);
		ASSERT(i < n);
		return slots[i].value();
	}

	T& operator[](int i) {
		ASSERT(i >= 0);
		ASSERT(i < n);
		return slots[i].value();
	}
	
	void clear() {
		n = 0;
	}
	
	void append(const T& val) {
		ASSERT(n < N);
		new(slots[n].address()) T(val);
		n++;
	}
	
	template<typename... Args>
	T* alloc(Args&&... args) {
		ASSERT(n < N);
		auto result = new(slots[n].address()) T(args ...);
		n++;
		return result;
	}
	
	int offsetOf(T* t) {
		auto slot = (Slot<T>*) t;
		ASSERT(slot >= slots && slot < slots + n);
		return slot - slots;
	}
	
	bool tryAppend(const T& val) {
		if (n < N) {
			append(val);
			return true;
		} else {
			return false;
		}
	}
	
	T& peekFirst() {
		ASSERT(n > 0);
		return slots[0].reference();
	}
	
	T& peekLast() {
		ASSERT(n > 0);
		return slots[n-1].reference();
	}
	
	T pop() {
		ASSERT(n > 0);
		n--;
		auto result = slots[n].value();
		slots[n].release();
		return result;
	}
	
	void removeAt(int i) {
		ASSERT(i >= 0);
		ASSERT(i < n);
		for(int j=i+1; j<n; ++j) {
			slots[j-1] = slots[j];
		}
		n--;
		slots[n].release();
	}
	
	void insertAt(const T& val, int i) {
		ASSERT(i >= 0);
		ASSERT(i <= n);
		ASSERT(n < N);
		if (i == n) {
			append(val);
		} else {
			for(int j=n; j > i; --j) {
				slots[j] = slots[j-1];
			}
			slots[i].reference() = val;
			n++;
		}
	}
	
	int findFirst(const T& val) const {
		for(int i=0; i<n; ++i) {
			if (slots[i].value() == val) { return i; }
		}
		return -1;
	}
	
	int findLast(const T& val) const {
		for(int i=n-1; i>=0; --i) {
			if (slots[i].value() == val) { return i; }
		}
		return -1;
	}

	bool contains(const T& val) const {
		return findFirst(val) != -1;
	}
};

