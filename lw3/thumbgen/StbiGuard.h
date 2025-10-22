#pragma once

#include "stb_image.h"

class StbiGuard
{
public:
	explicit StbiGuard(unsigned char* pixels)
		: m_pixels(pixels)
	{
	}

	~StbiGuard()
	{
		if (m_pixels)
		{
			stbi_image_free(m_pixels);
		}
	}

	StbiGuard(const StbiGuard&) = delete;
	StbiGuard& operator=(const StbiGuard&) = delete;

private:
	unsigned char* m_pixels;
};