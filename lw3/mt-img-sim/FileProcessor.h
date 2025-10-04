#pragma once

#include <filesystem>
#include <vector>

namespace FileProcessor
{
bool IsImageFile(const std::filesystem::path& path);
std::vector<std::filesystem::path> CollectImageFiles(const std::string& dir);
} // namespace FileProcessor