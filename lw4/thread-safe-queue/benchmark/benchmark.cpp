#include "ThreadSafeQueue.h"

#include <benchmark/benchmark.h>
#include <boost/lockfree/queue.hpp>
#include <thread>
#include <vector>

static constexpr size_t ItemsPerThread = 100'000;
static constexpr size_t BoundedCapacity = 1024;

void BM_UnboundedThreadSafeQueue(benchmark::State& state)
{
	const size_t numProducers = state.range(0);
	const size_t numConsumers = state.range(1);
	const size_t totalItems = numProducers * ItemsPerThread;

	for (auto _ : state)
	{
		ThreadSafeQueue<int> queue(0);
		std::atomic<size_t> consumed{ 0 };
		std::vector<std::jthread> threads;
		threads.reserve(numProducers + numConsumers);

		for (size_t i = 0; i < numProducers; ++i)
		{
			threads.emplace_back([&queue] {
				for (size_t j = 0; j < ItemsPerThread; ++j)
				{
					queue.Push(static_cast<int>(j));
				}
			});
		}

		for (size_t i = 0; i < numConsumers; ++i)
		{
			threads.emplace_back([&queue, &consumed, totalItems] {
				while (consumed.load(std::memory_order_relaxed) < totalItems)
				{
					if (queue.TryPop())
					{
						consumed.fetch_add(1, std::memory_order_relaxed);
					}
				}
			});
		}

		for (auto& t : threads)
		{
			t.join();
		}
		benchmark::DoNotOptimize(consumed.load());
	}
}

void BM_BoundedThreadSafeQueue(benchmark::State& state)
{
	const size_t numProducers = state.range(0);
	const size_t numConsumers = state.range(1);
	const size_t totalItems = numProducers * ItemsPerThread;

	for (auto _ : state)
	{
		ThreadSafeQueue<int> q(BoundedCapacity);
		std::atomic<size_t> consumed{ 0 };
		std::vector<std::jthread> threads;
		threads.reserve(numProducers + numConsumers);

		for (size_t i = 0; i < numProducers; ++i)
		{
			threads.emplace_back([&q] {
				for (size_t j = 0; j < ItemsPerThread; ++j)
				{
					q.Push(static_cast<int>(j));
				}
			});
		}

		for (size_t i = 0; i < numConsumers; ++i)
		{
			threads.emplace_back([&q, &consumed, totalItems] {
				while (consumed.load(std::memory_order_relaxed) < totalItems)
				{
					if (q.TryPop())
					{
						consumed.fetch_add(1, std::memory_order_relaxed);
					}
					else
					{
						std::this_thread::yield();
					}
				}
			});
		}

		for (auto& t : threads)
		{
			t.join();
		}
		benchmark::DoNotOptimize(consumed.load());
	}
}

void BM_BoostLockfreeQueue(benchmark::State& state)
{
	const size_t numProducers = state.range(0);
	const size_t numConsumers = state.range(1);
	const size_t totalItems = numProducers * ItemsPerThread;

	for (auto _ : state)
	{
		const size_t boostСapacity = totalItems + 1024;
		boost::lockfree::queue<int> queue(boostСapacity);
		std::atomic<size_t> consumed{ 0 };
		std::vector<std::jthread> threads;
		threads.reserve(numProducers + numConsumers);

		for (size_t i = 0; i < numProducers; ++i)
		{
			threads.emplace_back([&queue] {
				for (size_t j = 0; j < ItemsPerThread; ++j)
				{
					while (!queue.push(static_cast<int>(j)))
					{
					}
				}
			});
		}

		for (size_t i = 0; i < numConsumers; ++i)
		{
			threads.emplace_back([&queue, &consumed, totalItems] {
				while (consumed.load(std::memory_order_relaxed) < totalItems)
				{
					if (int val; queue.pop(val))
					{
						consumed.fetch_add(1, std::memory_order_relaxed);
					}
				}
			});
		}

		for (auto& t : threads)
		{
			t.join();
		}
		benchmark::DoNotOptimize(consumed.load());
	}
}

BENCHMARK(BM_UnboundedThreadSafeQueue)
	->Args({ 1, 1 })
	->Args({ 2, 2 })
	->Args({ 4, 4 })
	->Args({ 8, 8 })
	->Threads(1);

BENCHMARK(BM_BoundedThreadSafeQueue)
	->Args({ 1, 1 })
	->Args({ 2, 2 })
	->Args({ 4, 4 })
	->Args({ 8, 8 })
	->Threads(1);

BENCHMARK(BM_BoostLockfreeQueue)
	->Args({ 1, 1 })
	->Args({ 2, 2 })
	->Args({ 4, 4 })
	->Args({ 8, 8 })
	->Threads(1);

BENCHMARK_MAIN();