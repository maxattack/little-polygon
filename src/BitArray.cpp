#include "littlepolygon/collections.h"

BitArray::BitArray(unsigned cap) : capacity(cap) {
	words = (uint32_t*) calloc(nwords(), sizeof(uint32_t));
}

BitArray::~BitArray() {
	free(words);
}

void BitArray::clear() {
	SDL_memset(words, 0, nwords() * sizeof(uint32_t));
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

#if __WINDOWS__
inline uint32_t CLZ(uint32_t value)
{
	DWORD leading_zero = 0;
	return _BitScanReverse(&leading_zero, value) ? 31 - leading_zero : 32;
}
#else
#define CLZ(x) __builtin_clz(x)
#endif

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
