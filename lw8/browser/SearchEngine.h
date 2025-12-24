#pragma once

#include "InvertedIndex.h"
#include "ThreadPool.h"

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>

namespace fs = std::filesystem;

using ActionMap = std::unordered_map<std::string, std::function<void(std::istringstream&)>>;

class SearchEngine
{
public:
	explicit SearchEngine(std::istream& input, std::ostream& output,
		size_t threadCount = std::thread::hardware_concurrency());

	void Run();

private:
	void HandleCommand(const std::string& line);
	void AddFile(std::istringstream& args);
	void AddDirectory(std::istringstream& args, bool recursive);
	void Find(std::istringstream& args) const;
	void FindSubstring(std::istringstream& args) const;
	void FindBatch(std::istringstream& args) const;
	void RemoveFile(std::istringstream& args);
	void RemoveDirectory(std::istringstream& args, bool recursive);
	void PrintIndexedDocuments() const;

	static std::vector<std::filesystem::path> CollectFiles(
	const std::filesystem::path& dir, bool recursive);

	void AddFiles(const std::vector<std::filesystem::path>& files, size_t& outAddedCount);
	void ProcessBatchQueries(const std::vector<std::string>& queries) const;

	std::istream& m_input;
	std::ostream& m_output;
	ActionMap m_actionMap;
	std::unique_ptr<ThreadPool> m_threadPool;
	InvertedIndex m_index;
	std::atomic<std::uint64_t> m_nextDocId{ 1 };
	mutable std::mutex m_outputMutex;
};