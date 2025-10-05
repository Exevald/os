#pragma once

#include <optional>
#include <string>

namespace ArgsParser
{
struct Args
{
	std::string inputDirPath;
	std::string outputDirPath;
	int width = 256;
	int height = 256;
	int numberOfThreads = 1;
};

std::optional<Args> ParseArgs(int argc, char* argv[]);
} // namespace ArgsParser