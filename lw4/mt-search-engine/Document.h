#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

struct Document
{
	std::uint64_t id;
	std::string path;
	std::size_t wordCount = 0;
	std::unordered_map<std::string, std::size_t> termFrequencies;
};