#include "FileProcessor.h"

bool FileProcessor::IsImageFile(const std::filesystem::path& path)
{
	auto extension = path.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(),
		[](unsigned char c) { return std::tolower(c); });

	return extension == ".png" || extension == ".jpg" || extension == ".jpeg";
}

std::vector<std::filesystem::path> FileProcessor::CollectImageFiles(const std::string& dir)
{
	std::vector<std::filesystem::path> files;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
	{
		if (entry.is_regular_file() && IsImageFile(entry.path()))
		{
			files.push_back(entry.path());
		}
	}
	std::sort(files.begin(), files.end());

	return files;
}