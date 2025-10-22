#include "ImageProcessor.h"

#include "StbiGuard.h"
#include "stb_image.h"

#include <cmath>
#include <stdexcept>

namespace
{
float ToLinear(float srgb)
{
	return std::pow(srgb / 255.0f, 2.2f);
}
float FromLinear(float linear)
{
	return std::pow(linear, 1.0f / 2.2f) * 255.0f;
}
} // namespace

ImageProcessor::ImageRGB ImageProcessor::LoadImageAsRGB(const std::string& path)
{
	constexpr int desiredChannels = 3;
	int w, h, channels;

	unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &channels, desiredChannels);
	if (!pixels)
	{
		throw std::runtime_error("Failed to load image: " + path);
	}
	StbiGuard guard(pixels);

	ImageRGB img;
	img.width = w;
	img.height = h;
	img.data.assign(pixels, pixels + w * h * desiredChannels);

	return img;
}

ImageProcessor::ImageRGB ImageProcessor::ResizeBilinear(const ImageRGB& src, int outW, int outH)
{
	if (src.empty() || outW <= 0 || outH <= 0)
	{
		return {};
	}

	ImageRGB dst;
	dst.width = outW;
	dst.height = outH;
	dst.data.resize(outW * outH * 3);

	const float xRatio = (src.width > 1) ? static_cast<float>(src.width - 1) / static_cast<float>(outW - 1) : 0.0f;
	const float yRatio = (src.height > 1) ? static_cast<float>(src.height - 1) / static_cast<float>(outH - 1) : 0.0f;

	for (int y = 0; y < outH; ++y)
	{
		for (int x = 0; x < outW; ++x)
		{
			const float fx = (outW == 1) ? 0.0f : static_cast<float>(x) * xRatio;
			const float fy = (outH == 1) ? 0.0f : static_cast<float>(y) * yRatio;

			const int x1 = static_cast<int>(fx);
			const int y1 = static_cast<int>(fy);
			const int x2 = std::min(x1 + 1, src.width - 1);
			const int y2 = std::min(y1 + 1, src.height - 1);

			const float dx = fx - static_cast<float>(x1);
			const float dy = fy - static_cast<float>(y1);

			for (int c = 0; c < 3; ++c)
			{
				const float p11 = src.data[(y1 * src.width + x1) * 3 + c];
				const float p12 = src.data[(y1 * src.width + x2) * 3 + c];
				const float p21 = src.data[(y2 * src.width + x1) * 3 + c];
				const float p22 = src.data[(y2 * src.width + x2) * 3 + c];

				const float top = p11 * (1 - dx) + p12 * dx;
				const float bot = p21 * (1 - dx) + p22 * dx;
				const float val = top * (1 - dy) + bot * dy;

				dst.data[(y * outW + x) * 3 + c] = std::lround(val + 0.5f);
			}
		}
	}

	return dst;
}

std::vector<float> ImageProcessor::ApplyGammaToLinear(const ImageRGB& img)
{
	std::vector<float> linear(img.data.size());
	for (size_t i = 0; i < img.data.size(); ++i)
	{
		linear[i] = ToLinear(img.data[i]);
	}

	return linear;
}

std::vector<unsigned char> ImageProcessor::ConvertLinearToSRGB(const std::vector<float>& linear)
{
	std::vector<unsigned char> srgb(linear.size());
	for (size_t i = 0; i < linear.size(); ++i)
	{
		float val = FromLinear(linear[i]);
		val = std::max(0.0f, std::min(255.0f, val));
		srgb[i] = static_cast<unsigned char>(std::lround(val));
	}
	return srgb;
}

double ImageProcessor::ComputeMSE(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b)
{
	if (a.empty())
	{
		return 0.0;
	}

	double sum = 0.0;
	for (size_t i = 0; i < a.size(); ++i)
	{
		const double diff = a[i] - b[i];
		sum += diff * diff;
	}

	return sum / static_cast<double>(a.size());
}