#include "ArgsParser.h"

#include <iostream>
#include <sstream>
#include <string>

namespace
{
bool ParseThumbnailSize(const std::string& sizeStr, int& outWidth, int& outHeight)
{
	const auto pos = sizeStr.find('x');
	if (pos == std::string::npos || pos == 0 || pos == sizeStr.size() - 1)
	{
		std::cerr << "Error: invalid size format. Use WxH (e.g., 256x256)" << std::endl;
		return false;
	}
	try
	{
		outWidth = std::stoi(sizeStr.substr(0, pos));
		outHeight = std::stoi(sizeStr.substr(pos + 1));
		if (outWidth <= 0 || outHeight <= 0)
		{
			std::cerr << "Error: width and height must be positive" << std::endl;
			return false;
		}
	}
	catch (const std::exception&)
	{
		std::cerr << "Error: invalid size format" << std::endl;
		return false;
	}
	return true;
}
} // namespace

std::optional<ArgsParser::Args> ArgsParser::ParseArgs(int argc, char* argv[])
{
	if (argc < 3)
	{
		std::cerr << "Usage: thumbgen INPUT_DIR OUTPUT_DIR --size=WxH -j NUM_THREADS" << std::endl;
		return std::nullopt;
	}

	Args args;
	args.inputDirPath = argv[1];
	args.outputDirPath = argv[2];

	for (int i = 3; i < argc; ++i)
	{

		if (std::string arg = argv[i]; arg == "-j")
		{
			if (++i >= argc)
			{
				std::cerr << "Error: -j requires an argument" << std::endl;
				return std::nullopt;
			}
			try
			{
				args.numberOfThreads = std::stoi(argv[i]);
				if (args.numberOfThreads <= 0)
				{
					std::cerr << "Error: NUM_THREADS must be >= 1" << std::endl;
					return std::nullopt;
				}
			}
			catch (...)
			{
				std::cerr << "Error: invalid NUM_THREADS" << std::endl;
				return std::nullopt;
			}
		}
		else if (arg == "--size")
		{
			if (++i >= argc)
			{
				std::cerr << "Error: --size requires an argument (e.g., 128x128)" << std::endl;
				{
					return std::nullopt;
				}
			}
			if (!ParseThumbnailSize(argv[i], args.width, args.height))
			{
				return std::nullopt;
			}
		}
		else if (arg.rfind("--size=", 0) == 0)
		{
			if (std::string sizeStr = arg.substr(7); !ParseThumbnailSize(sizeStr, args.width, args.height))
			{
				return std::nullopt;
			}
		}
		else
		{
			std::cerr << "Unknown argument: " << arg << std::endl;
			{
				return std::nullopt;
			}
		}
	}

	return args;
}