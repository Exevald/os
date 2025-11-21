#pragma once

#include <cstdint>
#include <future>
#include <thread>
#include <vector>

class MipMapGenerator
{
public:
	static void GenerateLevel(const uint8_t* source, uint8_t* target,
		uint32_t sourceWidth, uint32_t sourceHeight,
		uint32_t tileWidth, uint32_t tileHeight);

	static std::future<void> GenerateMipMapsAsync(const std::vector<std::vector<uint8_t>>& sourceLevels,
		std::vector<std::vector<uint8_t>>& targetLevels,
		uint32_t tileWidth, uint32_t tileHeight);

	static void UpdateMipChain(const std::vector<std::vector<uint8_t>>& levels,
		uint32_t baseLevel, uint32_t baseX, uint32_t baseY,
		uint32_t tileWidth, uint32_t tileHeight);
};