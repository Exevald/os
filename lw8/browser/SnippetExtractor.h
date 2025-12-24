#pragma once
#include <string>
#include <vector>

namespace SnippetExtractor
{
std::string Extract(const std::string& content, const std::vector<std::string>& queryWords, size_t maxWords = 30);
}