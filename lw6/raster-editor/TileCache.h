#pragma once

#include <boost/scope_exit.hpp>
#include <cassert>
#include <functional>
#include <list>
#include <unordered_map>
#include <utility>

template <typename Key, typename Value>
class TileCache
{
public:
	explicit TileCache(size_t capacity) noexcept;
	~TileCache() = default;

	TileCache(const TileCache& other);
	TileCache(TileCache&& other) noexcept(
		std::is_nothrow_move_constructible_v<Value> && std::is_nothrow_move_constructible_v<Key>);
	TileCache& operator=(const TileCache& other);
	TileCache& operator=(TileCache&& other) noexcept(
		std::is_nothrow_move_assignable_v<Value> && std::is_nothrow_move_assignable_v<Key>);

	Value* Get(const Key& key) noexcept;
	void Put(const Key& key, const Value& value);
	Value* GetValueOrDefault(const Key& key, std::function<Value()> factory);

private:
	void UpdateIfKeyExists(const Key& key, const Value& value);
	void RemoveOverflowIfExists() noexcept;

	size_t m_capacity;
	std::list<std::pair<Key, Value>> m_itemsList;
	std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> m_itemsMap;
};

template <typename Key, typename Value>
TileCache<Key, Value>::TileCache(size_t capacity) noexcept
	: m_capacity(capacity)
{
	assert(m_capacity > 0);
}

template <typename Key, typename Value>
TileCache<Key, Value>::TileCache(const TileCache& other)
	: m_capacity(other.m_capacity)
{
	for (const auto& kv : other.m_itemsList)
	{
		m_itemsList.push_back(kv);
	}
	for (auto it = m_itemsList.begin(); it != m_itemsList.end(); ++it)
	{
		m_itemsMap[it->first] = it;
	}
}

template <typename Key, typename Value>
TileCache<Key, Value>::TileCache(TileCache&& other) noexcept(
	std::is_nothrow_move_constructible_v<Value> && std::is_nothrow_move_constructible_v<Key>)
	: m_capacity(other.m_capacity)
	, m_itemsList(std::move(other.m_itemsList))
	, m_itemsMap(std::move(other.m_itemsMap))
{
	other.m_capacity = 0;
	other.m_itemsMap.clear();
	other.m_itemsList.clear();
}

template <typename Key, typename Value>
TileCache<Key, Value>& TileCache<Key, Value>::operator=(const TileCache& other)
{
	if (this == &other)
	{
		return *this;
	}
	m_capacity = other.m_capacity;
	std::list<std::pair<Key, Value>> newList;
	for (const auto& kv : other.m_itemsList)
	{
		newList.push_back(kv);
	}
	std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> newMap;
	for (auto it = newList.begin(); it != newList.end(); ++it)
	{
		newMap[it->first] = it;
	}
	m_itemsList = std::move(newList);
	m_itemsMap = std::move(newMap);

	return *this;
}

template <typename Key, typename Value>
TileCache<Key, Value>& TileCache<Key, Value>::operator=(TileCache&& other) noexcept(
	std::is_nothrow_move_assignable_v<Value> && std::is_nothrow_move_assignable_v<Key>)
{
	if (this == &other)
	{
		return *this;
	}
	m_capacity = other.m_capacity;
	m_itemsList = std::move(other.m_itemsList);
	m_itemsMap = std::move(other.m_itemsMap);
	other.m_capacity = 0;
	other.m_itemsList.clear();
	other.m_itemsMap.clear();

	return *this;
}

template <typename Key, typename Value>
Value* TileCache<Key, Value>::Get(const Key& key) noexcept
{
	auto it = m_itemsMap.find(key);
	if (it == m_itemsMap.end())
	{
		return nullptr;
	}
	m_itemsList.splice(m_itemsList.begin(), m_itemsList, it->second);

	return &(it->second->second);
}

template <typename Key, typename Value>
void TileCache<Key, Value>::Put(const Key& key, const Value& value)
{
	UpdateIfKeyExists(key, value);
	RemoveOverflowIfExists();
	m_itemsList.emplace_front(key, value);

	BOOST_SCOPE_EXIT_ALL(&)
	{
		if (!m_itemsMap.contains(key))
		{
			m_itemsList.pop_front();
		}
	};

	m_itemsMap[key] = m_itemsList.begin();
}

template <typename Key, typename Value>
Value* TileCache<Key, Value>::GetValueOrDefault(const Key& key, std::function<Value()> factory)
{
	auto it = m_itemsMap.find(key);
	if (it != m_itemsMap.end())
	{
		m_itemsList.splice(m_itemsList.begin(), m_itemsList, it->second);
		return &it->second->second;
	}
	Value newValue = factory();

	RemoveOverflowIfExists();
	m_itemsList.emplace_front(key, std::move(newValue));

	bool commit = false;
	BOOST_SCOPE_EXIT_ALL(&)
	{
		if (!commit)
		{
			m_itemsList.pop_front();
		}
	};

	m_itemsMap[key] = m_itemsList.begin();
	commit = true;
	return &m_itemsList.begin()->second;
}

template <typename Key, typename Value>
void TileCache<Key, Value>::UpdateIfKeyExists(const Key& key, const Value& value)
{
	auto it = m_itemsMap.find(key);
	if (it != m_itemsMap.end())
	{
		it->second->second = value;
		m_itemsList.splice(m_itemsList.begin(), m_itemsList, it->second);
	}
}

template <typename Key, typename Value>
void TileCache<Key, Value>::RemoveOverflowIfExists() noexcept
{
	if (m_itemsList.size() == m_capacity)
	{
		const Key& lruKey = m_itemsList.back().first;
		m_itemsMap.erase(lruKey);
		m_itemsList.pop_back();
	}
}
