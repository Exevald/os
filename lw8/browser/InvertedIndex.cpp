#include "InvertedIndex.h"
#include "Tokenizer.h"

#include <cmath>
#include <filesystem>
#include <mutex>
#include <ranges>

namespace fs = std::filesystem;

namespace
{
bool IsPathInDir(const std::string& filePath, const std::string& dirPath)
{
	const fs::path file(filePath);
	const fs::path dir(dirPath);
	try
	{
		return file.parent_path() == dir;
	}
	catch (...)
	{
		return false;
	}
}

bool IsPathInDirRecursive(const std::string& filePath, const std::string& dirPath)
{
	const fs::path file(filePath);
	const fs::path dir(dirPath);
	try
	{
		return file.lexically_relative(dir).empty() || !file.lexically_relative(dir).has_root_path();
	}
	catch (...)
	{
		return false;
	}
}

template <typename T>
void LimitResults(std::vector<T>& results)
{
	constexpr size_t maxCount = 10;
	if (results.size() > maxCount)
	{
		results.resize(maxCount);
	}
}

} // namespace

InvertedIndex::InvertedIndex(int ngramSize)
	: m_ngramSize(ngramSize)
{
}

void InvertedIndex::AddDocument(std::uint64_t docId, const std::string& path, const std::string& content)
{
	const auto words = Tokenizer::ExtractWords(content);
	if (words.empty())
	{
		return;
	}

	Document doc;
	doc.id = docId;
	doc.path = path;
	doc.wordCount = words.size();

	std::unordered_map<std::string, std::size_t> tf;
	for (const auto& word : words)
	{
		tf[word]++;
	}
	doc.termFrequencies = std::move(tf);

	std::unique_lock lock(m_mutex);

	if (const auto it = m_pathToId.find(path); it != m_pathToId.end())
	{
		RemoveDocumentInternal(it->second);
	}

	m_documents[docId] = std::move(doc);
	m_pathToId[path] = docId;

	for (const auto& term : m_documents[docId].termFrequencies | std::views::keys)
	{
		m_termToDocs[term].insert(docId);
		for (auto grams = Tokenizer::GenerateNGrams(term, m_ngramSize);
			const auto& gram : grams)
		{
			m_ngramToDocs[gram].insert(docId);
		}
	}
	m_totalDocs = m_documents.size();
}

void InvertedIndex::RemoveDocument(const std::string& path)
{
	std::unique_lock lock(m_mutex);
	const auto it = m_pathToId.find(path);
	if (it == m_pathToId.end())
	{
		return;
	}

	RemoveDocumentInternal(it->second);
	m_pathToId.erase(it);
	m_totalDocs = m_documents.size();
}

void InvertedIndex::RemoveDocumentsInDir(const std::string& dirPath, bool recursive)
{
	std::vector<std::string> toRemove;
	{
		std::shared_lock lock(m_mutex);
		for (const auto& path : m_pathToId | std::views::keys)
		{
			const bool match = recursive
				? IsPathInDirRecursive(path, dirPath)
				: IsPathInDir(path, dirPath);
			if (match)
			{
				toRemove.push_back(path);
			}
		}
	}
	for (const auto& path : toRemove)
	{
		RemoveDocument(path);
	}
}

std::vector<std::pair<std::uint64_t, double>> InvertedIndex::Search(const std::vector<std::string>& queryTerms) const
{
	if (queryTerms.empty())
	{
		return {};
	}

	std::shared_lock lock(m_mutex);

	std::unordered_set<std::uint64_t> candidateDocs;
	for (const auto& term : queryTerms)
	{
		if (auto it = m_termToDocs.find(term); it != m_termToDocs.end())
		{
			candidateDocs.insert(it->second.begin(), it->second.end());
		}
	}
	if (candidateDocs.empty())
	{
		return {};
	}

	std::vector<std::pair<std::uint64_t, double>> results;
	results.reserve(candidateDocs.size());

	for (std::uint64_t docId : candidateDocs)
	{
		if (double score = ComputeRelevance(docId, queryTerms, m_totalDocs); score > 0.0)
		{
			results.emplace_back(docId, score);
		}
	}

	std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
	LimitResults(results);

	return results;
}

std::vector<std::uint64_t> InvertedIndex::SearchSubstring(const std::string& substring) const
{
	if (substring.empty())
	{
		return {};
	}

	std::string lowerSub = substring;
	std::transform(lowerSub.begin(), lowerSub.end(), lowerSub.begin(), [](unsigned char c) { return std::tolower(c); });

	const auto queryNGrams = Tokenizer::GenerateNGrams(lowerSub, m_ngramSize);
	if (queryNGrams.empty())
	{
		return {};
	}

	std::shared_lock lock(m_mutex);

	auto resultDocs = IntersectNgramResults(queryNGrams);
	LimitResults(resultDocs);

	return resultDocs;
}

std::string InvertedIndex::GetPathById(std::uint64_t id) const
{
	std::shared_lock lock(m_mutex);
	const auto it = m_documents.find(id);
	return (it != m_documents.end()) ? it->second.path : "";
}

bool InvertedIndex::HasDocument(const std::string& path) const
{
	std::shared_lock lock(m_mutex);
	return m_pathToId.contains(path);
}

std::vector<Document> InvertedIndex::GetIndexedDocuments() const
{
	std::shared_lock lock(m_mutex);
	std::vector<Document> docs;
	docs.reserve(m_documents.size());

	for (const auto& doc : m_documents | std::views::values)
	{
		docs.push_back(doc);
	}

	std::sort(docs.begin(), docs.end(), [](const Document& a, const Document& b) {
		return a.id < b.id;
	});

	return docs;
}

double InvertedIndex::ComputeRelevance(std::uint64_t docId, const std::vector<std::string>& queryTerms, std::size_t totalDocs) const
{
	const auto& doc = m_documents.at(docId);
	double score = 0.0;

	for (const auto& term : queryTerms)
	{
		auto tfIt = doc.termFrequencies.find(term);
		if (tfIt == doc.termFrequencies.end())
		{
			continue;
		}

		const double tf = static_cast<double>(tfIt->second) / static_cast<double>(doc.wordCount);

		auto dfIt = m_termToDocs.find(term);
		if (dfIt == m_termToDocs.end())
		{
			continue;
		}

		const std::size_t df = dfIt->second.size();
		if (df == 0 || totalDocs == 0)
		{
			continue;
		}

		const double idf = std::log(static_cast<double>(totalDocs) / static_cast<double>(df));
		score += tf * idf;
	}

	return score;
}

std::vector<std::uint64_t> InvertedIndex::IntersectNgramResults(
	const std::vector<std::string>& ngrams) const
{
	if (ngrams.empty())
	{
		return {};
	}

	const auto it = m_ngramToDocs.find(ngrams[0]);
	if (it == m_ngramToDocs.end())
	{
		return {};
	}

	std::vector resultDocs(it->second.begin(), it->second.end());
	std::sort(resultDocs.begin(), resultDocs.end());

	for (size_t i = 1; i < ngrams.size(); ++i)
	{
		auto ngramIt = m_ngramToDocs.find(ngrams[i]);
		if (ngramIt == m_ngramToDocs.end())
		{
			return {};
		}

		std::vector nextDocs(ngramIt->second.begin(), ngramIt->second.end());
		std::sort(nextDocs.begin(), nextDocs.end());

		std::vector<std::uint64_t> intersection;
		std::set_intersection(
			resultDocs.begin(), resultDocs.end(),
			nextDocs.begin(), nextDocs.end(),
			std::back_inserter(intersection));
		resultDocs = std::move(intersection);

		if (resultDocs.empty())
		{
			break;
		}
	}

	return resultDocs;
}

void InvertedIndex::RemoveDocumentInternal(std::uint64_t docId)
{
	const auto docIt = m_documents.find(docId);
	if (docIt == m_documents.end())
	{
		return;
	}
	for (const auto& [id, path, wordCount, termFrequencies] = docIt->second;
		const auto& term : termFrequencies | std::views::keys)
	{
		auto& docSet = m_termToDocs[term];
		docSet.erase(docId);
		if (docSet.empty())
		{
			m_termToDocs.erase(term);
		}

		for (auto grams = Tokenizer::GenerateNGrams(term, m_ngramSize);
			const auto& gram : grams)
		{
			auto& gramSet = m_ngramToDocs[gram];
			gramSet.erase(docId);
			if (gramSet.empty())
			{
				m_ngramToDocs.erase(gram);
			}
		}
	}

	m_documents.erase(docIt);
}