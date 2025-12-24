#include "Tokenizer.h"

std::vector<std::string> Tokenizer::ExtractWords(const std::string& text)
{
	std::vector<std::string> words;
	std::string current;
	for (const char ch : text)
	{
		if (std::isalpha(static_cast<unsigned char>(ch)))
		{
			current += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
		}
		else
		{
			if (!current.empty())
			{
				words.push_back(std::move(current));
				current.clear();
			}
		}
	}
	if (!current.empty())
	{
		words.push_back(std::move(current));
	}
	return words;
}

std::vector<std::string> Tokenizer::GenerateNGrams(const std::string& s, int n)
{
	std::vector<std::string> grams;
	if (s.size() < static_cast<size_t>(n))
	{
		if (!s.empty())
		{
			grams.push_back(s);
		}
		return grams;
	}
	for (size_t i = 0; i <= s.size() - n; ++i)
	{
		grams.push_back(s.substr(i, n));
	}
	return grams;
}