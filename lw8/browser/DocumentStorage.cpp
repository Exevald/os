#include "DocumentStorage.h"

void DocumentStorage::Add(std::uint64_t id, std::string url, std::string content)
{
	std::unique_lock lock(m_mutex);
	m_docs[id] = {std::move(url), std::move(content), ""};
}

std::optional<DocumentStorage::StoredDoc> DocumentStorage::Get(std::uint64_t id) const
{
	std::shared_lock lock(m_mutex);
	auto it = m_docs.find(id);
	return (it != m_docs.end()) ? std::make_optional(it->second) : std::nullopt;
}

bool DocumentStorage::Has(const std::string& url) const
{
	std::shared_lock lock(m_mutex);
	return std::any_of(m_docs.begin(), m_docs.end(),
		[&](const auto& p) { return p.second.url == url; });
}

void DocumentStorage::RemoveByURL(const std::string& url)
{
	std::unique_lock lock(m_mutex);
	auto it = std::find_if(m_docs.begin(), m_docs.end(),
		[&](const auto& p) { return p.second.url == url; });
	if (it != m_docs.end())
		m_docs.erase(it);
}

std::vector<std::pair<std::uint64_t, DocumentStorage::StoredDoc>> DocumentStorage::GetAll() const
{
	std::shared_lock lock(m_mutex);
	return {m_docs.begin(), m_docs.end()};
}