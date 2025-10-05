#include "ThumbnailGenerator.h"
#include "FileProcessor.h"
#include "ImageProcessor.h"
#include "stb_image_write.h"

#include <cmath>
#include <filesystem>
#include <stdexcept>

ThumbnailGenerator::ThumbnailGenerator(int width, int height)
	: m_width(width)
	, m_height(height)
{
}

bool ThumbnailGenerator::GenerateThumbnail(
	const std::filesystem::path& inputFilePath,
	const std::filesystem::path& inputDirPath,
	const std::filesystem::path& outputDirPath) const
{
	try
	{
		const auto img = ImageProcessor::LoadImageAsRGB(inputFilePath.string());
		const auto linear = ImageProcessor::ApplyGammaToLinear(img);

		ImageProcessor::ImageRGB linearImg;
		linearImg.width = img.width;
		linearImg.height = img.height;
		linearImg.data.resize(linear.size());

		for (size_t i = 0; i < linear.size(); ++i)
		{
			linearImg.data[i] = static_cast<unsigned char>(
				std::lround(std::clamp(linear[i] * 255.0f, 0.0f, 255.0f)));
		}

		const auto resizedLinear = ImageProcessor::ResizeBilinear(linearImg, m_width, m_height);
		std::vector<float> resizedLinearFloat(resizedLinear.data.size());
		for (size_t i = 0; i < resizedLinear.data.size(); ++i)
		{
			resizedLinearFloat[i] = static_cast<float>(resizedLinear.data[i]) / 255.0f;
		}
		const auto sRGBData = ImageProcessor::ConvertLinearToSRGB(resizedLinearFloat);

		const auto thumbnailPath = FileProcessor::GetThumbnailPath(inputFilePath, inputDirPath, outputDirPath);
		std::filesystem::create_directories(thumbnailPath.parent_path());

		if (!stbi_write_png(thumbnailPath.string().c_str(), m_width, m_height, 3, sRGBData.data(), 0))
		{
			throw std::runtime_error("stbi_write_png failed");
		}

		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
}