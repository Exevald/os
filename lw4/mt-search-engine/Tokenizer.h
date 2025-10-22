#pragma once

#include <string>
#include <vector>

namespace Tokenizer
{
std::vector<std::string> ExtractWords(const std::string& text);
std::vector<std::string> GenerateNGrams(const std::string& s, int n = 3);
} // namespace Tokenizer
