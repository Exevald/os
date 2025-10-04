#include "ArgsParser.h"
#include "FileProcessor.h"
#include "ImageProcessor.h"
#include "ThreadPool.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <future>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

struct Result
{
	double mse;
	std::filesystem::path path;
};

std::vector<unsigned char> PrepareQueryImage(const std::string& queryImagePath)
{
	const auto queryImg = ImageProcessor::LoadImageAsRGB(queryImagePath);
	const auto queryResized = ImageProcessor::ResizeBilinear(queryImg, 256, 256);
	const auto queryLinear = ImageProcessor::ApplyGammaToLinear(queryResized);
	return ImageProcessor::ConvertLinearToSRGB(queryLinear);
}

std::vector<Result> ProcessCandidateImages(
	const std::vector<std::filesystem::path>& files,
	const std::vector<unsigned char>& querySRGB,
	int numThreads)
{
	std::vector<std::future<Result>> tasks;
	ThreadPool pool(numThreads);

	for (const auto& path : files)
	{
		auto task = pool.Enqueue([querySRGB, path]() -> Result {
			try
			{
				const auto img = ImageProcessor::LoadImageAsRGB(path.string());
				const auto resized = ImageProcessor::ResizeBilinear(img, 256, 256);
				const auto linear = ImageProcessor::ApplyGammaToLinear(resized);
				const auto candidateSRGB = ImageProcessor::ConvertLinearToSRGB(linear);
				const double mse = ImageProcessor::ComputeMSE(querySRGB, candidateSRGB);
				return { mse, path };
			}
			catch (...)
			{
				return { std::numeric_limits<double>::max(), path };
			}
		});
		tasks.push_back(std::move(task));
	}

	std::vector<Result> validResults;
	for (auto& task : tasks)
	{
		if (auto result = task.get(); result.mse != std::numeric_limits<double>::max())
		{
			validResults.push_back(result);
		}
	}

	return validResults;
}

std::vector<Result> FilterResults(const std::vector<Result>& results, const ArgsParser::Args& args)
{
	auto sorted = results;
	std::ranges::sort(sorted, [](const Result& a, const Result& b) {
		return a.mse < b.mse;
	});

	std::vector<Result> output;
	for (const auto& result : sorted)
	{
		const bool byTopCount = (args.topCount == 0) || (static_cast<int>(output.size()) < args.topCount);
		if (const bool byThreshold = (args.threshold < 0) || (result.mse <= args.threshold);
			byTopCount && byThreshold)
		{
			output.push_back(result);
		}
		else if (!byTopCount)
		{
			break;
		}
	}

	return output;
}

void PrintResults(const std::vector<Result>& results)
{
	for (const auto& [mse, path] : results)
	{
		std::cout << mse << "  " << path << std::endl;
	}
}

void FindSimilarImages(const ArgsParser::Args& args)
{
	const auto queryLinear = PrepareQueryImage(args.queryImagePath);

	const auto files = FileProcessor::CollectImageFiles(args.inputDirPath);
	if (files.empty())
	{
		return;
	}

	const auto validResults = ProcessCandidateImages(files, queryLinear, args.numberOfThreads);
	const auto output = FilterResults(validResults, args);

	PrintResults(output);
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
		FindSimilarImages(*args);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 2;
	}

	return 0;
}