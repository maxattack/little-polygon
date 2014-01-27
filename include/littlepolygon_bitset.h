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
		reset(); 
	}

	inline static unsigned size() { return tSize; }
	inline bool empty() const { return nonzeroWords == 0; }

	void reset() {
		// return to an empty state, even if we're internally
		// inconsistent (e.g. if we're allocated with malloc)
		nonzeroWords = 0;
		memset(words, 0, sizeof(uint32_t) * tSize / 32);
	}

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
			index = clz(words[w]) | (w<<5);
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
			index |= (w<<5);
			return true;
		} else {
			return false;
		}
	}

	// TODO: C++11 style iterator?

	class iterator {
	private:
		const Bitset<tSize> *bs;
		uint32_t remainder;
		unsigned w, v;

	public:
		iterator() : remainder(0) {}
		iterator(const Bitset<tSize>* aSet) : bs(aSet) {
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
				index |= (w<<5);
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

	iterator listBits() const { return iterator(this); }

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

