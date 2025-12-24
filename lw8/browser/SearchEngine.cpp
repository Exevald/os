#include "SearchEngine.h"
#include "Tokenizer.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

namespace
{
std::optional<std::string> ReadFile(const std::filesystem::path& p)
{
	std::ifstream file(p, std::ios::binary);
	if (!file)
	{
		return std::nullopt;
	}
	std::ostringstream ss;
	ss << file.rdbuf();
	return ss.str();
}
} // namespace

SearchEngine::SearchEngine(std::istream& input, std::ostream& output, size_t threadCount)
	: m_input(input)
	, m_output(output)
	, m_threadPool(std::make_unique<ThreadPool>(threadCount ? threadCount : 16))
	, m_index(3)
{
	m_actionMap.emplace("add_file", [this](std::istringstream& args) { AddFile(args); });
	m_actionMap.emplace("add_dir", [this](std::istringstream& args) { AddDirectory(args, false); });
	m_actionMap.emplace("add_dir_recursive", [this](std::istringstream& args) { AddDirectory(args, true); });
	m_actionMap.emplace("find", [this](std::istringstream& args) { Find(args); });
	m_actionMap.emplace("find_substring", [this](std::istringstream& args) { FindSubstring(args); });
	m_actionMap.emplace("find_batch", [this](std::istringstream& args) { FindBatch(args); });
	m_actionMap.emplace("remove_file", [this](std::istringstream& args) { RemoveFile(args); });
	m_actionMap.emplace("remove_dir", [this](std::istringstream& args) { RemoveDirectory(args, false); });
	m_actionMap.emplace("remove_dir_recursive", [this](std::istringstream& args) { RemoveDirectory(args, true); });
	m_actionMap.emplace("print_indexed_documents", [this](std::istringstream& _) { PrintIndexedDocuments(); });
}

void SearchEngine::Run()
{
	std::string line;
	std::cout << ">";
	while (std::getline(m_input, line))
	{
		if (!line.empty())
		{
			HandleCommand(line);
			std::cout << ">";
		}
	}
}

void SearchEngine::HandleCommand(const std::string& line)
{
	std::istringstream iss(line);
	std::string command;
	iss >> command;

	if (const auto it = m_actionMap.find(command); it != m_actionMap.end())
	{
		try
		{
			it->second(iss);
		}
		catch (const std::exception& e)
		{
			m_output << "error: " << e.what() << std::endl;
		}
		catch (...)
		{
			m_output << "error: unknown exception" << std::endl;
		}
		return;
	}
	m_output << "error: unknown command" << std::endl;
}

void SearchEngine::AddFile(std::istringstream& args)
{
	std::string path;
	args >> path;
	if (path.empty())
	{
		m_output << "error: empty query" << std::endl;
		return;
	}
	const fs::path p = fs::absolute(fs::path(path));
	if (!fs::exists(p) || !fs::is_regular_file(p))
	{
		m_output << "error: path not found: " << path << std::endl;
		return;
	}
	const auto content = ReadFile(p);
	if (!content)
	{
		m_output << "error: cannot read file: " << path << std::endl;
		return;
	}

	const std::uint64_t id = m_nextDocId.fetch_add(1);
	m_index.AddDocument(id, p.string(), *content);
}

void SearchEngine::AddDirectory(std::istringstream& args, bool recursive)
{
	std::string path;
	args >> path;
	if (path.empty())
	{
		m_output << "error: empty path" << std::endl;
		return;
	}

	const auto dir = std::filesystem::absolute(std::filesystem::path(path));
	if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
	{
		m_output << "error: path not found: " << path << std::endl;
		return;
	}

	const auto start = std::chrono::high_resolution_clock::now();
	std::vector<std::filesystem::path> files;
	try
	{
		files = CollectFiles(dir, recursive);
	}
	catch (const std::exception& e)
	{
		m_output << "error: " << e.what() << std::endl;
		return;
	}

	if (files.empty())
	{
		m_output << "No files to add." << std::endl;
		return;
	}

	size_t addedCount = 0;
	AddFiles(files, addedCount);

	const auto end = std::chrono::high_resolution_clock::now();
	const double duration = std::chrono::duration<double>(end - start).count();

	m_output << "Adding took " << std::fixed << std::setprecision(4) << duration << "s:" << std::endl;
	m_output << "Added " << addedCount << " file(s) from directory: " << dir.string() << std::endl;
}

void SearchEngine::Find(std::istringstream& args) const
{
	std::string query;
	std::getline(args, query);
	if (!query.empty() && query[0] == ' ')
	{
		query.erase(0, 1);
	}
	if (query.empty())
	{
		m_output << "error: empty query" << std::endl;
		return;
	}

	const auto terms = Tokenizer::ExtractWords(query);
	if (terms.empty())
	{
		m_output << "error: empty query" << std::endl;
		return;
	}

	const auto start = std::chrono::high_resolution_clock::now();
	auto results = m_index.Search(terms);
	const auto end = std::chrono::high_resolution_clock::now();
	const double duration = std::chrono::duration<double>(end - start).count();

	m_output << "Search took " << std::fixed << std::setprecision(4) << duration << "s:" << std::endl;
	for (size_t i = 0; i < results.size(); ++i)
	{
		const auto& [id, relevance] = results[i];
		m_output << (i + 1) << ". id:" << id
				 << ", relevance:" << std::fixed << std::setprecision(5) << relevance
				 << ", path:" << m_index.GetPathById(id) << std::endl;
	}
	if (!results.empty())
	{
		m_output << "---" << std::endl;
	}
}

void SearchEngine::FindSubstring(std::istringstream& args) const
{
	std::string substring;
	std::getline(args, substring);
	if (!substring.empty() && substring[0] == ' ')
	{
		substring.erase(0, 1);
	}
	if (substring.empty())
	{
		m_output << "error: empty query" << std::endl;
		return;
	}

	const auto start = std::chrono::high_resolution_clock::now();
	const auto docIds = m_index.SearchSubstring(substring);
	const auto end = std::chrono::high_resolution_clock::now();
	const double duration = std::chrono::duration<double>(end - start).count();

	m_output << "Substring search took " << std::fixed << std::setprecision(4) << duration << "s:" << std::endl;
	for (size_t i = 0; i < docIds.size(); ++i)
	{
		m_output << (i + 1) << ". id:" << docIds[i]
				 << ", path:" << m_index.GetPathById(docIds[i]) << std::endl;
	}
	if (!docIds.empty())
	{
		m_output << "---" << std::endl;
	}
}

void SearchEngine::FindBatch(std::istringstream& args) const
{
	std::string path;
	args >> path;
	if (path.empty())
	{
		m_output << "error: empty path" << std::endl;
		return;
	}

	const auto p = std::filesystem::absolute(std::filesystem::path(path));
	if (!std::filesystem::exists(p) || !std::filesystem::is_regular_file(p))
	{
		m_output << "error: path not found: " << path << std::endl;
		return;
	}

	std::ifstream file(p);
	if (!file)
	{
		m_output << "error: cannot open file: " << path << std::endl;
		return;
	}

	std::vector<std::string> queries;
	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty())
		{
			queries.push_back(line);
		}
	}

	if (queries.empty())
	{
		m_output << "No queries found in file." << std::endl;
		return;
	}

	m_output << "Processing " << queries.size() << " query(ies) from: " << p.string() << std::endl;
	ProcessBatchQueries(queries);
}

void SearchEngine::RemoveFile(std::istringstream& args)
{
	std::string path;
	args >> path;
	if (path.empty())
	{
		m_output << "error: empty query" << std::endl;
		return;
	}

	fs::path p = fs::absolute(fs::path(path));
	if (!m_index.HasDocument(p.string()))
	{
		m_output << "error: file not in index: " << path << std::endl;
		return;
	}
	m_index.RemoveDocument(p.string());
}

void SearchEngine::RemoveDirectory(std::istringstream& args, bool recursive)
{
	std::string path;
	args >> path;
	if (path.empty())
	{
		m_output << "error: empty query" << std::endl;
		return;
	}

	const fs::path dir = fs::absolute(fs::path(path));
	if (!fs::exists(dir) || !fs::is_directory(dir))
	{
		m_output << "error: path not found: " << path << std::endl;
		return;
	}
	m_index.RemoveDocumentsInDir(dir.string(), recursive);
}

void SearchEngine::PrintIndexedDocuments() const
{
	for (const auto docs = m_index.GetIndexedDocuments(); const auto& doc : docs)
	{
		m_output << doc.path << std::endl;
	}
}

std::vector<std::filesystem::path> SearchEngine::CollectFiles(const std::filesystem::path& dir, bool recursive)
{
	std::vector<std::filesystem::path> files;
	try
	{
		if (recursive)
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
			{
				if (entry.is_regular_file())
				{
					files.push_back(entry.path());
				}
			}
		}
		else
		{
			for (const auto& entry : std::filesystem::directory_iterator(dir))
			{
				if (entry.is_regular_file())
				{
					files.push_back(entry.path());
				}
			}
		}
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		throw std::runtime_error("path not found: " + dir.string() + " (" + e.what() + ")");
	}
	return files;
}

void SearchEngine::AddFiles(const std::vector<std::filesystem::path>& files, size_t& outAddedCount)
{
	std::atomic<size_t> added{ 0 };

	for (const auto& file : files)
	{
		m_threadPool->Enqueue([this, file, &added]() {
			const auto content = ReadFile(file);
			if (!content)
			{
				return;
			}
			const std::uint64_t id = m_nextDocId.fetch_add(1);
			m_index.AddDocument(id, file.string(), *content);
			++added;
		});
	}

	m_threadPool->Wait();
	outAddedCount = added.load();
}

void SearchEngine::ProcessBatchQueries(const std::vector<std::string>& queries) const
{
	for (size_t i = 0; i < queries.size(); ++i)
	{
		const size_t idx = i + 1;
		const std::string& q = queries[i];
		m_threadPool->Enqueue([this, idx, q]() {
			const auto terms = Tokenizer::ExtractWords(q);
			if (terms.empty())
			{
				return;
			}

			const auto start = std::chrono::high_resolution_clock::now();
			auto results = m_index.Search(terms);
			const auto end = std::chrono::high_resolution_clock::now();
			const double duration = std::chrono::duration<double>(end - start).count();

			std::lock_guard lock(m_outputMutex);
			m_output << idx << ". query: " << q << std::endl;
			m_output << "  Search took " << std::fixed << std::setprecision(4) << duration << "s:" << std::endl;
			for (size_t j = 0; j < results.size(); ++j)
			{
				const auto& [id, relevance] = results[j];
				m_output << "  " << (j + 1) << ". id:" << id
						 << ", relevance:" << std::fixed << std::setprecision(5) << relevance
						 << ", path:" << m_index.GetPathById(id) << std::endl;
			}
			if (!results.empty())
			{
				m_output << "  ---" << std::endl;
			}
		});
	}

	m_threadPool->Wait();
}