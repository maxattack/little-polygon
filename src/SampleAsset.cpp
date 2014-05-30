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

#include "littlepolygon/assets.h"
#include <zlib.h>

struct WaveHeader {
	uint8_t ChunkId[4];
	uint32_t ChunkSize;
	uint8_t Format[4];
	
	uint8_t Subchunk1ID[4];
	uint32_t Subchunk1Size;
	uint16_t AudioFormat;
	uint16_t NumChannels;
	uint32_t SampleRate;
	uint32_t ByteRate;
	uint16_t BlockAlign;
	uint16_t BitsPerSample;
	
	uint8_t Subchunk2ID[4];
	uint32_t Subchunk2Size;
	
	void init(int numChannels, int sampleRate, int sampleWidth, int numSamples) {
		NumChannels = (uint16_t) numChannels;
		SampleRate = sampleRate;
		
		ByteRate = sampleRate * numChannels * sampleWidth;
		BlockAlign = (uint16_t) (numChannels * sampleWidth);
		BitsPerSample = (uint16_t) (sampleWidth<<3);
		Subchunk2Size = numSamples * numChannels * sampleWidth;
		ChunkSize = 36 + Subchunk2Size;
	}
};

void SampleAsset::init() {
	if (chunk == 0) {
		// Allocate a buffer for the RW_ops structure to read from 
		Bytef *scratch = (Bytef*) SDL_malloc(size + sizeof(WaveHeader));
		{
		// Mixer expects a WAVE header on PCM data, so let's provide it :P
		WaveHeader hdr = {{'R','I','F','F'},0,{'W','A','V','E'},{'f','m','t',' '},16,1,1,0,0,0,0,{'d','a','t','a'},0};
		int sampleCount = size / (sampleWidth * channelCount);
		hdr.init(channelCount, frequency, sampleWidth, sampleCount);
		memcpy(scratch, &hdr, sizeof(WaveHeader));
		}
		// Now decompress the actual PCM data
		uLongf sz = size;
		uncompress(
			scratch + sizeof(WaveHeader), 
			&sz, 
			(const Bytef*)compressedData, 
			compressedSize
		);
		// load the chunk
		chunk = Mix_LoadWAV_RW(SDL_RWFromMem(scratch, sz+sizeof(WaveHeader)), 1);
		ASSERT(chunk);
		SDL_free(scratch);
	}
}

void SampleAsset::release() {
	if (chunk) {
		Mix_FreeChunk(chunk);
		chunk = 0;
	}
}

void SampleAsset::play() {
	init();
	Mix_PlayChannel(-1, chunk, 0);
}
