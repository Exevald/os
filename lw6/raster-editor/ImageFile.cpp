#include "ImageFile.h"
#include "MortonOrder.h"
#include <boost/scope_exit.hpp>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

ImageFile::ImageFile(std::string filename)
	: m_filename(std::move(filename))
{
	InitializeFile();
}

ImageFile::~ImageFile()
{
	Flush();
}

void ImageFile::InitializeFile()
{
	try
	{
		m_fileMapping = boost::interprocess::file_mapping(
			m_filename.c_str(),
			boost::interprocess::read_write);

		boost::interprocess::mapped_region headerRegion(m_fileMapping, boost::interprocess::read_write, 0, sizeof(ImageHeader));
		std::memcpy(&m_header, headerRegion.get_address(), sizeof(ImageHeader));
	}
	catch (const boost::interprocess::interprocess_exception& ex)
	{
	}
}

void ImageFile::CreateImage(uint32_t width, uint32_t height, uint32_t tileWidth, uint32_t tileHeight, uint32_t mipLevels)
{
	m_header.width = width;
	m_header.height = height;
	m_header.tileWidth = tileWidth;
	m_header.tileHeight = tileHeight;

	if (mipLevels == 0)
	{
		CalculateMipLevels();
	}
	else
	{
		m_header.mipLevels = mipLevels;
	}

	uint64_t totalFileSize = sizeof(ImageHeader);
	uint32_t currentWidth = width;
	uint32_t currentHeight = height;

	for (uint32_t level = 0; level < m_header.mipLevels; ++level)
	{
		uint32_t tilesX = (currentWidth + tileWidth - 1) / tileWidth;
		uint32_t tilesY = (currentHeight + tileHeight - 1) / tileHeight;
		uint64_t levelSize = static_cast<uint64_t>(tilesX) * tilesY * tileWidth * tileHeight * 4;
		totalFileSize += levelSize;

		currentWidth /= 2;
		currentHeight /= 2;
	}

	int fd = open(m_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
	{
		throw std::runtime_error("Failed to create file: " + m_filename);
	}

	BOOST_SCOPE_EXIT_ALL(&)
	{
		close(fd);
	};

	if (ftruncate(fd, static_cast<off_t>(totalFileSize)) == -1)
	{
		throw std::runtime_error("Failed to resize file");
	}

	close(fd);

	m_fileMapping = boost::interprocess::file_mapping(m_filename.c_str(), boost::interprocess::read_write);

	boost::interprocess::mapped_region headerRegion(m_fileMapping, boost::interprocess::read_write, 0, sizeof(ImageHeader));
	std::memcpy(headerRegion.get_address(), &m_header, sizeof(ImageHeader));
	headerRegion.flush();
}

void ImageFile::LoadImage(const std::string& filename)
{
	m_filename = filename;
	m_fileMapping = boost::interprocess::file_mapping(m_filename.c_str(), boost::interprocess::read_write);

	boost::interprocess::mapped_region headerRegion(m_fileMapping, boost::interprocess::read_only, 0, sizeof(ImageHeader));
	std::memcpy(&m_header, headerRegion.get_address(), sizeof(ImageHeader));
}

boost::interprocess::mapped_region* ImageFile::GetTileRegion(int32_t tileX, int32_t tileY, uint32_t level)
{
	if (!IsValidTile(tileX, tileY, level))
	{
		return nullptr;
	}

	uint64_t offset = GetTileOffset(tileX, tileY, level);
	size_t tileSize = GetTileSize();

	for (auto& region : m_mappedRegions)
	{
		if (region.get_address() != nullptr
			&& static_cast<uint64_t>(reinterpret_cast<char*>(region.get_address()) - reinterpret_cast<char*>(m_fileMapping.get_mapping_handle())) == offset)
		{
			return &region;
		}
	}

	try
	{
		m_mappedRegions.emplace_back(m_fileMapping, boost::interprocess::read_write, offset, tileSize);
		return &m_mappedRegions.back();
	}
	catch (const boost::interprocess::interprocess_exception& ex)
	{
		return nullptr;
	}
}

void ImageFile::WriteTile(int32_t tileX, int32_t tileY, uint32_t level, const void* data, size_t size)
{
	if (!IsValidTile(tileX, tileY, level))
	{
		return;
	}

	uint64_t offset = GetTileOffset(tileX, tileY, level);
	size_t tileSize = GetTileSize();

	if (size > tileSize)
	{
		throw std::runtime_error("Tile data exceeds tile size");
	}

	boost::interprocess::mapped_region tempRegion(m_fileMapping, boost::interprocess::read_write, offset, tileSize);
	std::memcpy(tempRegion.get_address(), data, size);
	tempRegion.flush();
}

void ImageFile::Flush()
{
	for (auto& region : m_mappedRegions)
	{
		if (region.get_address() != nullptr)
		{
			region.flush();
		}
	}
}

uint64_t ImageFile::GetTileOffset(int32_t tileX, int32_t tileY, uint32_t level) const
{
	return CalculateFileOffset(tileX, tileY, level);
}

bool ImageFile::IsValidTile(int32_t tileX, int32_t tileY, uint32_t level) const
{
	if (level >= m_header.mipLevels)
	{
		return false;
	}

	uint32_t levelWidth = m_header.width >> level;
	uint32_t levelHeight = m_header.height >> level;

	if (levelWidth == 0)
		levelWidth = 1;
	if (levelHeight == 0)
		levelHeight = 1;

	uint32_t tilesX = (levelWidth + m_header.tileWidth - 1) / m_header.tileWidth;
	uint32_t tilesY = (levelHeight + m_header.tileHeight - 1) / m_header.tileHeight;

	return tileX >= 0 && static_cast<uint32_t>(tileX) < tilesX && tileY >= 0 && static_cast<uint32_t>(tileY) < tilesY;
}

size_t ImageFile::GetTileSize() const
{
	return static_cast<size_t>(m_header.tileWidth) * m_header.tileHeight * 4; // 4 байта на пиксель (RGBA)
}

std::pair<uint32_t, uint32_t> ImageFile::GetTileCount(uint32_t level) const
{
	if (level >= m_header.mipLevels)
	{
		return { 0, 0 };
	}

	uint32_t levelWidth = m_header.width >> level;
	uint32_t levelHeight = m_header.height >> level;

	if (levelWidth == 0)
		levelWidth = 1;
	if (levelHeight == 0)
		levelHeight = 1;

	uint32_t tilesX = (levelWidth + m_header.tileWidth - 1) / m_header.tileWidth;
	uint32_t tilesY = (levelHeight + m_header.tileHeight - 1) / m_header.tileHeight;

	return { tilesX, tilesY };
}

void ImageFile::CalculateMipLevels()
{
	uint32_t maxDimension = std::max(m_header.width, m_header.height);
	m_header.mipLevels = 1;
	while (maxDimension > m_header.tileWidth)
	{
		maxDimension /= 2;
		m_header.mipLevels++;
	}
}

uint64_t ImageFile::CalculateFileOffset(int32_t tileX, int32_t tileY, uint32_t level) const
{
	uint64_t offset = sizeof(ImageHeader);

	uint32_t currentWidth = m_header.width;
	uint32_t currentHeight = m_header.height;

	for (uint32_t l = 0; l < level; ++l)
	{
		uint32_t tilesX = (currentWidth + m_header.tileWidth - 1) / m_header.tileWidth;
		uint32_t tilesY = (currentHeight + m_header.tileHeight - 1) / m_header.tileHeight;
		offset += static_cast<uint64_t>(tilesX) * tilesY * GetTileSize();

		currentWidth /= 2;
		currentHeight /= 2;
	}

	uint32_t levelWidth = m_header.width >> level;
	uint32_t levelHeight = m_header.height >> level;

	if (levelWidth == 0)
		levelWidth = 1;
	if (levelHeight == 0)
		levelHeight = 1;

	uint32_t tilesX = (levelWidth + m_header.tileWidth - 1) / m_header.tileWidth;

	uint32_t mortonIndex = MortonEncode(tileX, tileY);

	uint32_t actualTilesY = (levelHeight + m_header.tileHeight - 1) / m_header.tileHeight;
	if (tileX >= tilesX || tileY >= actualTilesY)
	{
		return offset;
	}

	uint64_t tileOffset = (static_cast<uint64_t>(tileY) * tilesX + tileX) * GetTileSize();

	return offset + tileOffset;
}