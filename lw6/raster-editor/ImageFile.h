#pragma once

#include <array>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/scope_exit.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct ImageHeader
{
	uint32_t width;
	uint32_t height;
	uint32_t tileWidth;
	uint32_t tileHeight;
	uint32_t mipLevels;
	uint32_t padding[11];
};

struct TileKey
{
	int32_t x;
	int32_t y;
	uint32_t level;

	bool operator==(const TileKey& other) const
	{
		return x == other.x && y == other.y && level == other.level;
	}

	bool operator<(const TileKey& other) const
	{
		if (level != other.level)
			return level < other.level;
		if (x != other.x)
			return x < other.x;
		return y < other.y;
	}
};

namespace std
{
template <>
struct hash<TileKey>
{
	size_t operator()(const TileKey& key) const
	{
		size_t h1 = hash<int32_t>{}(key.x);
		size_t h2 = hash<int32_t>{}(key.y);
		size_t h3 = hash<uint32_t>{}(key.level);
		return h1 ^ (h2 << 1) ^ (h3 << 2);
	}
};
} // namespace std

class ImageFile
{
public:
	explicit ImageFile(std::string filename);
	~ImageFile();

	void CreateImage(uint32_t width, uint32_t height, uint32_t tileWidth = 32, uint32_t tileHeight = 32, uint32_t mipLevels = 0);
	void LoadImage(const std::string& filename);

	boost::interprocess::mapped_region* GetTileRegion(int32_t tileX, int32_t tileY, uint32_t level = 0);

	void WriteTile(int32_t tileX, int32_t tileY, uint32_t level, const void* data, size_t size);

	void Flush();

	const ImageHeader& GetHeader() const { return m_header; }

	uint64_t GetTileOffset(int32_t tileX, int32_t tileY, uint32_t level) const;

	bool IsValidTile(int32_t tileX, int32_t tileY, uint32_t level) const;

	size_t GetTileSize() const;

	std::pair<uint32_t, uint32_t> GetTileCount(uint32_t level = 0) const;

private:
	std::string m_filename;
	ImageHeader m_header{};
	boost::interprocess::file_mapping m_fileMapping;
	std::vector<boost::interprocess::mapped_region> m_mappedRegions;
	std::mutex m_mutex;

	void InitializeFile();
	void CalculateMipLevels();
	uint64_t CalculateFileOffset(int32_t tileX, int32_t tileY, uint32_t level) const;
};