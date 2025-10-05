#pragma once

#include <string>
#include <vector>

namespace ImageProcessor
{
struct ImageRGB
{
	int width = 0, height = 0;
	std::vector<unsigned char> data;
	[[nodiscard]] bool empty() const { return data.empty(); }
};

ImageRGB LoadImageAsRGB(const std::string& path);
ImageRGB ResizeBilinear(const ImageRGB& src, int outW, int outH);
std::vector<float> ApplyGammaToLinear(const ImageRGB& img);
std::vector<unsigned char> ConvertLinearToSRGB(const std::vector<float>& linear);
double ComputeMSE(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b);
} // namespace ImageProcessor