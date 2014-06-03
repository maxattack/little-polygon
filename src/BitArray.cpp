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

#include "littlepolygon/collections.h"

BitArray::BitArray(unsigned cap) :
capacity(cap), words(cap)
{
}

void BitArray::clear()
{
	memset(words.ptr(), 0, nwords() * sizeof(uint32_t));
}

void BitArray::clear(unsigned i)
{
	unsigned word, local; getIndices(i, &word, &local);
	words[word] &= ~bit(local);
}

void BitArray::mark(unsigned i)
{
	unsigned word, local; getIndices(i, &word, &local);
	words[word] |= bit(local);
}

bool BitArray::operator[](unsigned i) const
{
	unsigned word, local; getIndices(i, &word, &local);
	return (words[word] & bit(local)) != 0;
}

void BitArray::getIndices(unsigned idx, unsigned* outWord, unsigned *outIndex) const
{
	*outWord = idx >> 5;
	*outIndex = idx % 32;

}

uint32_t BitArray::bit(unsigned i)
{
	return 0x80000000 >> i;
}

BitLister::BitLister(const BitArray *arr) :
pArray(arr),
currentWord(-1),
currentIndex(-1),
remainder(0)
{
}

bool BitLister::next()
{
	if (remainder) {
		currentIndex = CLZ(remainder);
		remainder ^= BitArray::bit(currentIndex);
		return true;
	} else {
		do {
			currentWord++;
		} while(currentWord < pArray->nwords() && pArray->words[currentWord] == 0);
		if (currentWord < pArray->nwords()) {
			remainder = pArray->words[currentWord];
			currentIndex = CLZ(remainder);
			remainder ^= BitArray::bit(currentIndex);
			return true;
		} else {
			currentIndex = -1;
			return false;
		}
	}
}
