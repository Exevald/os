#include "FileProcessor.h"

#include <algorithm>

bool FileProcessor::IsImageFile(const std::filesystem::path& path)
{
	auto extension = path.extension().string();
	std::ranges::transform(extension, extension.begin(),
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
	std::ranges::sort(files);

	return files;
}

std::filesystem::path FileProcessor::GetThumbnailPath(
	const std::filesystem::path& inputFilePath,
	const std::filesystem::path& inputDirPath,
	const std::filesystem::path& outputDirPath)
{
	const auto relative = std::filesystem::relative(inputFilePath, inputDirPath);
	const auto filename = relative.stem().string() + "_thumbnail";
	const auto directory = relative.parent_path();
	return outputDirPath / directory / (filename + ".png");
}