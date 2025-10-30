#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <shared_mutex>

template <typename T>
class ThreadSafeQueue
{
public:
	explicit ThreadSafeQueue(size_t capacity = 0)
		: m_capacity(capacity)
	{
	}

	void Push(const T& value)
	{
		std::unique_lock lock(m_queueMutex);
		if (m_capacity > 0)
		{
			m_notFull.wait(lock, [&] { return m_queue.size() < m_capacity; });
		}
		m_queue.emplace_back(value);
		m_notEmpty.notify_one();
	}
	void Push(T&& value)
	{
		std::unique_lock lock(m_queueMutex);
		if (m_capacity > 0)
		{
			m_notFull.wait(lock, [&] { return m_queue.size() < m_capacity; });
		}
		m_queue.emplace_back(std::move(value));
		m_notEmpty.notify_one();
	}

	[[nodiscard]] bool TryPush(const T& value)
	{
		std::unique_lock lock(m_queueMutex);
		if (m_capacity > 0 && m_queue.size() >= m_capacity)
		{
			return false;
		}
		m_queue.emplace_back(value);
		m_notEmpty.notify_one();
		return true;
	}
	[[nodiscard]] bool TryPush(T&& value)
	{
		std::unique_lock lock(m_queueMutex);
		if (m_capacity > 0 && m_queue.size() >= m_capacity)
		{
			return false;
		}
		m_queue.emplace_back(std::move(value));
		m_notEmpty.notify_one();
		return true;
	}

	bool TryPop(T& out)
	{
		std::unique_lock lock(m_queueMutex);
		if (m_queue.empty())
		{
			return false;
		}
		out = std::move(m_queue.front());
		m_queue.pop_front();
		if (m_capacity > 0)
		{
			m_notFull.notify_one();
		}
		return true;
	}
	std::unique_ptr<T> TryPop()
	{
		std::unique_lock lock(m_queueMutex);
		if (m_queue.empty())
		{
			return nullptr;
		}
		auto out = std::make_unique<T>(std::move(m_queue.front()));
		m_queue.pop_front();
		if (m_capacity > 0)
		{
			m_notFull.notify_one();
		}
		return out;
	}

	template <typename U = T>
	std::enable_if_t<std::is_nothrow_move_constructible_v<U>, U>
	WaitAndPop()
	{
		std::unique_lock lock(m_queueMutex);
		m_notEmpty.wait(lock, [&] { return !m_queue.empty(); });
		auto value = std::move(m_queue.front());
		m_queue.pop_front();
		if (m_capacity > 0)
		{
			m_notFull.notify_one();
		}
		return value;
	}

	void WaitAndPop(T& out)
	{
		std::unique_lock lock(m_queueMutex);
		m_notEmpty.wait(lock, [&] { return !m_queue.empty(); });
		out = std::move(m_queue.front());
		m_queue.pop_front();
		if (m_capacity > 0)
		{
			m_notFull.notify_one();
		}
	}

	size_t GetSize() const
	{
		std::shared_lock lock(m_queueMutex);
		return m_queue.size();
	}
	size_t GetCapacity() const
	{
		std::shared_lock lock(m_queueMutex);
		return m_capacity;
	}
	bool IsEmpty() const
	{
		std::shared_lock lock(m_queueMutex);
		return m_queue.empty();
	}

	void Swap(ThreadSafeQueue& other)
	{
		if (this == &other)
		{
			return;
		}

		ThreadSafeQueue* first = this;
		ThreadSafeQueue* second = &other;
		if (first > second)
		{
			std::swap(first, second);
		}

		std::scoped_lock lock(first->m_queueMutex, second->m_queueMutex);
		std::swap(first->m_queue, second->m_queue);
		std::swap(first->m_capacity, second->m_capacity);

		first->m_notEmpty.notify_all();
		second->m_notEmpty.notify_all();
		first->m_notFull.notify_all();
		second->m_notFull.notify_all();
	}
	void Swap(std::deque<T>& other)
	{
		std::unique_lock lock(m_queueMutex);
		std::swap(m_queue, other);
		m_notEmpty.notify_all();
		if (m_capacity > 0)
		{
			m_notFull.notify_all();
		}
	}

private:
	mutable std::shared_mutex m_queueMutex;
	std::deque<T> m_queue;
	size_t m_capacity;
	std::condition_variable_any m_notEmpty;
	std::condition_variable_any m_notFull;
};