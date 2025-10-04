#pragma once

#include <optional>
#include <string>

namespace ArgsParser
{
struct Args
{
	std::string queryImagePath;
	std::string inputDirPath;
	int numberOfThreads = 1;
	int topCount = 0;
	double threshold = -1.0;
};

std::optional<Args> ParseArgs(int argc, char* argv[]);
} // namespace ArgsParser