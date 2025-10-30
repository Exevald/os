#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool
{
public:
	using Task = std::function<void()>;

	explicit ThreadPool(size_t numThreads)
	{
		m_workers.reserve(numThreads);
		for (size_t i = 0; i < numThreads; ++i)
		{
			m_workers.emplace_back(&ThreadPool::WorkerLoop, this);
		}
	}

	~ThreadPool()
	{
		m_stopFlag = true;
		m_stateChanged.notify_all();
	}

	template <class F, class... Args>
	auto Enqueue(F&& f, Args&&... args)
		-> std::future<std::invoke_result_t<F, Args...>>
	{
		using returnType = std::invoke_result_t<F, Args...>;

		auto task = std::make_shared<std::packaged_task<returnType()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		std::future<returnType> res = task->get_future();
		{
			std::unique_lock lock(m_queueMutex);
			if (m_stopFlag)
			{
				throw std::runtime_error("enqueue on stopped ThreadPool");
			}

			m_tasks.emplace([task = std::move(task)] {
				(*task)();
			});
			++m_activeTasks;
		}
		m_stateChanged.notify_one();
		return res;
	}

	void Wait()
	{
		std::unique_lock lock(m_queueMutex);
		m_tasksEmpty.wait(lock, [this] {
			// TODO: написать комментарий, почему тут memory_order_relaxed
			return m_tasks.empty() && m_activeTasks.load(std::memory_order_relaxed) == 0;
		});
	}

private:
	void WorkerLoop()
	{
		while (true)
		{
			Task task;
			{
				std::unique_lock lock(m_queueMutex);
				m_stateChanged.wait(lock, [this] {
					return m_stopFlag.load(std::memory_order_relaxed) || !m_tasks.empty();
				});

				if (m_stopFlag.load(std::memory_order_relaxed) && m_tasks.empty())
				{
					return;
				}

				task = std::move(m_tasks.front());
				m_tasks.pop();
			}

			try
			{
				task();
			}
			catch (...)
			{
			}

			m_activeTasks.fetch_sub(1, std::memory_order_release);
			if (m_activeTasks.load(std::memory_order_acquire) == 0 && m_tasks.empty())
			{
				m_tasksEmpty.notify_all();
			}
		}
	}

	std::vector<std::jthread> m_workers;
	std::queue<Task> m_tasks;
	std::mutex m_queueMutex;
	std::condition_variable m_stateChanged;
	std::condition_variable m_tasksEmpty;
	std::atomic<bool> m_stopFlag{ false };
	std::atomic<size_t> m_activeTasks{ 0 };
};