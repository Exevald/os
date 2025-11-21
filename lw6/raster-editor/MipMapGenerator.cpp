#include "MipMapGenerator.h"

void MipMapGenerator::GenerateLevel(const uint8_t* source, uint8_t* target,
	uint32_t sourceWidth, uint32_t sourceHeight,
	uint32_t tileWidth, uint32_t tileHeight)
{
	uint32_t targetWidth = (sourceWidth + 1) / 2;
	uint32_t targetHeight = (sourceHeight + 1) / 2;

	for (uint32_t ty = 0; ty < targetHeight; ++ty)
	{
		for (uint32_t tx = 0; tx < targetWidth; ++tx)
		{
			uint32_t targetTileX = tx * tileWidth;
			uint32_t targetTileY = ty * tileHeight;

			for (uint32_t py = 0; py < tileHeight; ++py)
			{
				for (uint32_t px = 0; px < tileWidth; ++px)
				{
					uint32_t srcX = tx * 2 * tileWidth + px * 2;
					uint32_t srcY = ty * 2 * tileHeight + py * 2;

					uint32_t r = 0, g = 0, b = 0, a = 0;
					uint32_t count = 0;

					for (int dy = 0; dy < 2 && srcY + dy < sourceHeight; ++dy)
					{
						for (int dx = 0; dx < 2 && srcX + dx < sourceWidth; ++dx)
						{
							uint32_t srcIdx = ((srcY + dy) * sourceWidth + (srcX + dx)) * 4;
							r += source[srcIdx];
							g += source[srcIdx + 1];
							b += source[srcIdx + 2];
							a += source[srcIdx + 3];
							count++;
						}
					}

					if (count > 0)
					{
						uint32_t targetIdx = (targetTileY + py) * targetWidth * 4 + (targetTileX + px) * 4;
						target[targetIdx] = static_cast<uint8_t>(r / count);
						target[targetIdx + 1] = static_cast<uint8_t>(g / count);
						target[targetIdx + 2] = static_cast<uint8_t>(b / count);
						target[targetIdx + 3] = static_cast<uint8_t>(a / count);
					}
				}
			}
		}
	}
}

std::future<void> MipMapGenerator::GenerateMipMapsAsync(
	const std::vector<std::vector<uint8_t>>& sourceLevels,
	std::vector<std::vector<uint8_t>>& targetLevels,
	uint32_t tileWidth, uint32_t tileHeight)
{

	return std::async(std::launch::async, [&]() {
		for (size_t i = 1; i < targetLevels.size(); ++i)
		{
			GenerateLevel(sourceLevels[i - 1].data(), targetLevels[i].data(),
				sourceLevels[i - 1].size() / (tileWidth * 4),
				sourceLevels[i - 1].size() / (tileWidth * 4 * tileWidth),
				tileWidth, tileHeight);
		}
	});
}

void MipMapGenerator::UpdateMipChain(const std::vector<std::vector<uint8_t>>& levels,
	uint32_t baseLevel, uint32_t baseX, uint32_t baseY,
	uint32_t tileWidth, uint32_t tileHeight)
{
	uint32_t currentLevel = baseLevel;
	uint32_t currentX = baseX;
	uint32_t currentY = baseY;

	while (currentLevel + 1 < levels.size())
	{
		uint32_t parentX = currentX / 2;
		uint32_t parentY = currentY / 2;

		currentLevel++;
		currentX = parentX;
		currentY = parentY;
	}
}