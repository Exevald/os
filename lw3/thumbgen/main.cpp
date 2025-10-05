#include "ArgsParser.h"
#include "FileProcessor.h"
#include "StatsCounter.h"
#include "ThreadPool.h"
#include "ThumbnailGenerator.h"

#include <filesystem>
#include <future>
#include <iostream>
#include <vector>

void GenerateThumbnails(const std::optional<ArgsParser::Args>& args)
{
	const auto files = FileProcessor::CollectImageFiles(args->inputDirPath);
	if (files.empty())
	{
		std::cout << "processed=0 failed=0" << std::endl;
		return;
	}

	ThumbnailGenerator generator(args->width, args->height);
	StatsCounter stats;

	ThreadPool pool(args->numberOfThreads);
	std::vector<std::future<void>> tasks;
	tasks.reserve(files.size());

	for (const auto& file : files)
	{
		tasks.push_back(pool.Enqueue(
			[&generator, &stats, &file, &args]() {
				if (generator.GenerateThumbnail(file, args->inputDirPath, args->outputDirPath))
				{
					stats.IncrementProcessed();
				}
				else
				{
					stats.IncrementFailed();
					std::cerr << "Failed to process " << file << std::endl;
				}
			}));
	}

	for (auto& task : tasks)
	{
		task.wait();
	}

	std::cout << "processed=" << stats.GetProcessedFilesCount() << " failed=" << stats.GetFailedFilesCount() << std::endl;
}

int main(int argc, char* argv[])
{
	const auto args = ArgsParser::ParseArgs(argc, argv);
	if (!args)
	{
		return 1;
	}

	try
	{
		GenerateThumbnails(args);
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << std::endl;
		return 2;
	}
}