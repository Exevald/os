#pragma once

#include "Document.h"

#include <cstddef>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class InvertedIndex
{
public:
	explicit InvertedIndex(int ngramSize = 3);

	void AddDocument(std::uint64_t docId, const std::string& path, const std::string& content);
	void RemoveDocument(const std::string& path);
	void RemoveDocumentsInDir(const std::string& dirPath, bool recursive = false);
	std::vector<std::pair<std::uint64_t, double>> Search(const std::vector<std::string>& queryTerms) const;
	std::vector<std::uint64_t> SearchSubstring(const std::string& substring) const;
	std::string GetPathById(std::uint64_t id) const;
	bool HasDocument(const std::string& path) const;
	std::vector<Document> GetIndexedDocuments() const;

private:
	double ComputeRelevance(
		std::uint64_t docId,
		const std::vector<std::string>& queryTerms,
		std::size_t totalDocs) const;
	std::vector<std::uint64_t> IntersectNgramResults(const std::vector<std::string>& ngrams) const;
	void RemoveDocumentInternal(std::uint64_t docId);

	const int m_ngramSize;
	mutable std::shared_mutex m_mutex;
	std::unordered_map<std::uint64_t, Document> m_documents;
	std::unordered_map<std::string, std::uint64_t> m_pathToId;
	std::unordered_map<std::string, std::unordered_set<std::uint64_t>> m_termToDocs;
	std::unordered_map<std::string, std::unordered_set<std::uint64_t>> m_ngramToDocs;
	// зачем?
	std::size_t m_totalDocs = 0;
};