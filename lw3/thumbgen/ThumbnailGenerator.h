#pragma once

#include <filesystem>

class ThumbnailGenerator
{
public:
	ThumbnailGenerator(int width, int height);

	bool GenerateThumbnail(
		const std::filesystem::path& inputFilePath,
		const std::filesystem::path& inputDirPath,
		const std::filesystem::path& outputDirPath) const;

private:
	int m_width;
	int m_height;
};