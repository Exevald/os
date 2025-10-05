#pragma once

#include <filesystem>
#include <vector>

namespace FileProcessor
{
bool IsImageFile(const std::filesystem::path& path);
std::vector<std::filesystem::path> CollectImageFiles(const std::string& dir);
std::filesystem::path GetThumbnailPath(
	const std::filesystem::path& inputFilePath,
	const std::filesystem::path& inputDirPath,
	const std::filesystem::path& outputDirPath);
} // namespace FileProcessor