#include "ArgsParser.h"

#include <iostream>

std::optional<ArgsParser::Args> ArgsParser::ParseArgs(int argc, char* argv[])
{
	if (argc < 3)
	{
		std::cerr << "Usage: mt-img-sim QUERY_IMAGE INPUT_DIR -j NUM_THREADS --top K --threshold T\n";
		return std::nullopt;
	}

	Args args;
	args.queryImagePath = argv[1];
	args.inputDirPath = argv[2];

	for (int i = 3; i < argc; ++i)
	{
		if (std::string arg = argv[i]; arg == "-j")
		{
			if (++i >= argc)
			{
				return std::nullopt;
			}
			args.numberOfThreads = std::stoi(argv[i]);
			if (args.numberOfThreads <= 0)
			{
				return std::nullopt;
			}
		}
		else if (arg == "--top")
		{
			if (++i >= argc)
			{
				return std::nullopt;
			}
			args.topCount = std::stoi(argv[i]);
			if (args.topCount <= 0)
			{
				return std::nullopt;
			}
		}
		else if (arg == "--threshold")
		{
			if (++i >= argc)
			{
				return std::nullopt;
			}
			args.threshold = std::stod(argv[i]);
			if (args.threshold < 0)
			{
				return std::nullopt;
			}
		}
		else
		{
			std::cerr << "Unknown argument: " << arg << "\n";
			return std::nullopt;
		}
	}

	return args;
}
