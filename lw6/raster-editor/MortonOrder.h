#pragma once

#include <cstdint>

inline uint32_t MortonEncode(uint32_t x, uint32_t y)
{
	uint32_t answer = 0;
	for (int i = 0; i < 16; ++i)
	{
		answer |= ((x & (1u << i)) << i) | ((y & (1u << i)) << (i + 1));
	}
	return answer;
}

inline void MortonDecode(uint32_t morton, uint32_t& x, uint32_t& y)
{
	x = y = 0;
	for (int i = 0; i < 16; ++i)
	{
		x |= ((morton >> (2 * i)) & 1) << i;
		y |= ((morton >> (2 * i + 1)) & 1) << i;
	}
}