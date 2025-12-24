#pragma once

#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

class DocumentStorage
{
public:
	struct StoredDoc
	{
		std::string url;
		std::string content;
		std::string title;
	};

	void Add(std::uint64_t id, std::string url, std::string content);
	std::optional<StoredDoc> Get(std::uint64_t id) const;
	bool Has(const std::string& url) const;
	void RemoveByURL(const std::string& url);
	std::vector<std::pair<std::uint64_t, StoredDoc>> GetAll() const;

private:
	mutable std::shared_mutex m_mutex;
	std::unordered_map<std::uint64_t, StoredDoc> m_docs;
};