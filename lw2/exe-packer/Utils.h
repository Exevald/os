#pragma once

#include <iostream>

struct SFXHeader
{
	char signature[4];
	std::uint64_t originalSize;
	uint64_t compressedSize;
};

constexpr size_t HeaderSize = 4 + sizeof(uint64_t) * 2; // 20 bytes
constexpr char SFXSignature[] = "SFX!";