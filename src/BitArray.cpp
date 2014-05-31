#include "littlepolygon/collections.h"

BitArray::BitArray(unsigned cap) : capacity(cap) {
	words = (uint32_t*) SDL_calloc(nwords(), sizeof(uint32_t));
}

BitArray::~BitArray() {
	SDL_free(words);
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

bool BitLister::next()
{
	if (remainder) {
		currentIndex = __builtin_clz(remainder);
		remainder ^= BitArray::bit(currentIndex);
		return true;
	} else {
		do {
			currentWord++;
		} while(currentWord < pArray->nwords() && pArray->words[currentWord] == 0);
		if (currentWord < pArray->nwords()) {
			remainder = pArray->words[currentWord];
			currentIndex = __builtin_clz(remainder);
			remainder ^= BitArray::bit(currentIndex);
			return true;
		} else {
			currentIndex = -1;
			return false;
		}
	}
}