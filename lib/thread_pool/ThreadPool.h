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
		for (size_t i = 0; i < numThreads; ++i)
		{
			m_workers.emplace_back([this] {
				while (true)
				{
					Task task;
					{
						std::unique_lock lock(this->m_queueMutex);
						this->m_cv.wait(lock, [this] {
							return this->m_stopFlag || !this->m_tasks.empty();
						});
						if (this->m_stopFlag && this->m_tasks.empty())
						{
							return;
						}
						task = std::move(this->m_tasks.front());
						this->m_tasks.pop();
					}
					task();
					--this->m_activeTasks;
				}
			});
		}
	}

	~ThreadPool()
	{
		{
			std::unique_lock lock(m_queueMutex);
			m_stopFlag = true;
		}
		m_cv.notify_all();
		for (std::thread& worker : m_workers)
		{
			worker.join();
		}
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

			m_tasks.emplace([task] {
				(*task)();
			});
			++m_activeTasks;
		}
		m_cv.notify_one();

		return res;
	}

private:
	std::vector<std::thread> m_workers;
	std::queue<Task> m_tasks;
	std::mutex m_queueMutex;
	std::condition_variable m_cv;
	std::atomic<bool> m_stopFlag{ false };
	std::atomic<size_t> m_activeTasks{ 0 };
};