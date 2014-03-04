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
#include <utility>

//--------------------------------------------------------------------------------
// SIMPLE TEMPLATE QUEUE
//--------------------------------------------------------------------------------

template<typename T, int N>
class Queue {
private:
	int n;
	int i;
	union Slot {
		T record;
		Slot *next;

		Slot() {}
		~Slot() {}
	};	
	Slot slots[N];

public:
	Queue() : n(0), i(0) {}
	
	int capacity() const { return N; }
	int count() const { return n; }
	bool empty() const { return n == 0; }
	bool full() const { return n == N; }
	
	void enqueue(const T& val) {
		ASSERT(n < N);
		new(&slots[(i + n) % N].record) T(val);		
		n++;
	}
	
	template<typename... Args>
	void emplace(Args&&... args) {
		ASSERT(n < N);
		new(&slots[(i + n) % N].record) T(std::forward<Args>(args) ...);
		n++;
	}
	
	T dequeue() {
		ASSERT(n > 0);
		T& result = slots[i].record;
		n--;
		i = (i+1) % N;
		return result;
	}

	T peekNext() {
		ASSERT(n > 0);
		return slots[i].record;
	}
	
	T peekLast() {
		ASSERT(n > 0);
		return slots[(i+n-1) % N].record;
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
			*outValue = q->slots[(q->i + idx) % N].record;
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
	union Slot {
		T record;
		Slot *next;

		Slot() {}
		~Slot() {}
	};	
	Slot slots[N];

public:
	List() : n(0) {}
	
	int capacity() const { return N; }
	int count() const { return n; }
	size_t rawCapacity() const { return N * sizeof(T); }
	size_t rawSize() const { return n * sizeof(T); }
	bool empty() const { return n == 0; }
	bool full() const { return n == N; }

	T* begin() const { return (T*)slots; }
	T* end() const { return begin() + n; }
	
	T get(int i) const {
		ASSERT(i >= 0);
		ASSERT(i < n);
		return slots[i].record;
	}

	T& operator[](int i) {
		ASSERT(i >= 0);
		ASSERT(i < n);
		return slots[i].record;
	}
	
	void clear() {
		n = 0;
	}
	
	void append(const T& val) {
		ASSERT(n < N);
		new(&slots[n].record) T(val);
		n++;
	}
	
	template<typename... Args>
	T* alloc(Args&&... args) {
		ASSERT(n < N);
		auto result = new(&slots[n].record) T(std::forward<Args>(args) ...);
		n++;
		return result;
	}
	
	int offsetOf(T* t) {
		auto slot = (Slot*) t;
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
		return slots[0].record;
	}
	
	T& peekLast() {
		ASSERT(n > 0);
		return slots[n-1].record;
	}
	
	T& push() {
		ASSERT(n < N);
		n++;
		return slots[n-1].record;
	}
	
	T pop() {
		ASSERT(n > 0);
		n--;
		return slots[n].record;
	}
	
	void removeAt(int i) {
		ASSERT(i >= 0);
		ASSERT(i < n);
		for(int j=i+1; j<n; ++j) {
			slots[j-1].record = slots[j].record;
		}
		n--;
	}
	
	void insertAt(const T& val, int i) {
		ASSERT(i >= 0);
		ASSERT(i <= n);
		ASSERT(n < N);
		if (i == n) {
			append(val);
		} else {
			for(int j=n; j > i; --j) {
				slots[j].record = slots[j-1].record;
			}
			slots[i].record = val;
			n++;
		}
	}
	
	int findFirst(const T& val) const {
		for(int i=0; i<n; ++i) {
			if (slots[i].record == val) { return i; }
		}
		return -1;
	}
	
	int findLast(const T& val) const {
		for(int i=n-1; i>=0; --i) {
			if (slots[i].record == val) { return i; }
		}
		return -1;
	}

	bool contains(const T& val) const {
		return findFirst(val) != -1;
	}
};
