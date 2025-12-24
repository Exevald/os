#include "SnippetExtractor.h"
#include <cctype>
#include <algorithm>

std::string SnippetExtractor::Extract(const std::string& content, const std::vector<std::string>& queryWords, size_t maxWords)
{
	if (content.empty() || queryWords.empty()) return "...";

	std::vector<std::pair<size_t, size_t>> wordRanges;
	std::vector<std::string> cleanWords;

	size_t i = 0;
	while (i < content.size())
	{
		if (std::isalnum(static_cast<unsigned char>(content[i])))
		{
			size_t start = i;
			std::string word;
			while (i < content.size() && std::isalnum(static_cast<unsigned char>(content[i])))
			{
				word += static_cast<char>(std::tolower(static_cast<unsigned char>(content[i++])));
			}
			cleanWords.push_back(word);
			wordRanges.emplace_back(start, i);
		}
		else
		{
			++i;
		}
	}

	if (cleanWords.empty()) return "...";

	std::vector<size_t> matches;
	for (size_t idx = 0; idx < cleanWords.size(); ++idx)
	{
		if (std::find(queryWords.begin(), queryWords.end(), cleanWords[idx]) != queryWords.end())
		{
			matches.push_back(idx);
		}
	}

	if (matches.empty()) return "...";

	size_t bestStart = 0, bestEnd = std::min(maxWords, cleanWords.size());
	size_t minSpan = cleanWords.size();

	for (size_t wStart = 0; wStart < cleanWords.size(); ++wStart)
	{
		size_t covered = 0;
		for (size_t wEnd = wStart; wEnd < cleanWords.size() && (wEnd - wStart + 1) <= maxWords; ++wEnd)
		{
			if (std::find(queryWords.begin(), queryWords.end(), cleanWords[wEnd]) != queryWords.end())
				covered++;

			if (covered == queryWords.size() || (covered > 0 && wEnd - wStart + 1 == maxWords))
			{
				if (wEnd - wStart < minSpan)
				{
					minSpan = wEnd - wStart;
					bestStart = wStart;
					bestEnd = wEnd + 1;
				}
				break;
			}
		}
	}

	if (bestEnd - bestStart > maxWords)
		bestEnd = bestStart + maxWords;

	size_t startPos = wordRanges[bestStart].first;
	size_t endPos = (bestEnd <= wordRanges.size()) ? wordRanges[bestEnd - 1].second : content.size();

	std::string snippet = content.substr(startPos, endPos - startPos);

	if (snippet.size() > 300)
	{
		snippet = snippet.substr(0, 300);
		size_t lastSpace = snippet.find_last_of(" \t\n\r");
		if (lastSpace != std::string::npos)
			snippet = snippet.substr(0, lastSpace);
		snippet += "...";
	}

	return snippet;
}