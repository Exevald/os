#include "ThreadSafeQueue.h"
#include <chrono>
#include <gtest/gtest.h>
#include <stdexcept>
#include <thread>
#include <vector>

template <typename Predicate>
bool WaitWithTimeout(Predicate&& p, std::chrono::milliseconds timeout = std::chrono::seconds(5))
{
	const auto start = std::chrono::steady_clock::now();
	while (!p())
	{
		if (std::chrono::steady_clock::now() - start > timeout)
		{
			return false;
		}
		std::this_thread::yield();
	}
	return true;
}

TEST(ThreadSafeQueueTest, TryPopOnEmptyReturnsFalse)
{
	ThreadSafeQueue<int> queue;
	int out;
	EXPECT_FALSE(queue.TryPop(out));
	EXPECT_EQ(nullptr, queue.TryPop());
}

TEST(ThreadSafeQueueTest, PushThenTryPopReturnsSameValue)
{
	ThreadSafeQueue<int> queue;
	queue.Push(42);
	int out;
	EXPECT_TRUE(queue.TryPop(out));
	EXPECT_EQ(42, out);

	queue.Push(100);
	auto ptr = queue.TryPop();
	ASSERT_NE(nullptr, ptr);
	EXPECT_EQ(100, *ptr);
}

TEST(ThreadSafeQueueTest, GetSizeAndIsEmptyAreCorrect)
{
	ThreadSafeQueue<int> queue;
	EXPECT_TRUE(queue.IsEmpty());
	EXPECT_EQ(0, queue.GetSize());

	queue.Push(1);
	EXPECT_FALSE(queue.IsEmpty());
	EXPECT_EQ(1, queue.GetSize());

	queue.TryPop();
	EXPECT_TRUE(queue.IsEmpty());
	EXPECT_EQ(0, queue.GetSize());
}

TEST(ThreadSafeQueueTest, BoundedCapacityOneSecondTryPushFails)
{
	ThreadSafeQueue<int> queue(1);
	EXPECT_TRUE(queue.TryPush(10));
	EXPECT_FALSE(queue.TryPush(20));
	EXPECT_EQ(1, queue.GetSize());
}

TEST(ThreadSafeQueueTest, BoundedPushBlocksUntilPop)
{
	ThreadSafeQueue<int> queue(1);
	queue.Push(100);

	std::atomic producerDone{ false };
	std::jthread producer([&] {
		queue.Push(200);
		producerDone = true;
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	EXPECT_FALSE(producerDone.load());

	int val;
	EXPECT_TRUE(queue.TryPop(val));
	EXPECT_EQ(100, val);

	ASSERT_TRUE(WaitWithTimeout([&] { return producerDone.load(); }));
	EXPECT_TRUE(queue.TryPop(val));
	EXPECT_EQ(200, val);
}

TEST(ThreadSafeQueueTest, UnboundedPushDoesNotBlockAndPreservesFIFO)
{
	ThreadSafeQueue<int> q(0);
	constexpr size_t n = 1000;
	for (size_t i = 0; i < n; ++i)
	{
		q.Push(static_cast<int>(i));
	}
	EXPECT_EQ(n, q.GetSize());

	for (size_t i = 0; i < n; ++i)
	{
		int val;
		EXPECT_TRUE(q.TryPop(val));
		EXPECT_EQ(static_cast<int>(i), val);
	}
	EXPECT_TRUE(q.IsEmpty());
}

TEST(ThreadSafeQueueTest, MPMC_NoLossNoDuplication)
{
	constexpr size_t numProducers = 4;
	constexpr size_t numConsumers = 4;
	constexpr size_t itemsPerProducer = 1000;
	constexpr size_t totalItems = numProducers * itemsPerProducer;

	ThreadSafeQueue<int> queue;
	std::atomic<size_t> consumed{ 0 };
	std::vector<int> results;
	std::mutex resultsMutex;

	std::vector<std::jthread> threads;

	for (size_t i = 0; i < numProducers; ++i)
	{
		threads.emplace_back([&, id = i] {
			for (size_t j = 0; j < itemsPerProducer; ++j)
			{
				queue.Push(static_cast<int>(id * itemsPerProducer + j));
			}
		});
	}

	for (size_t i = 0; i < numConsumers; ++i)
	{
		threads.emplace_back([&] {
			while (consumed.load(std::memory_order_relaxed) < totalItems)
			{
				if (auto val = queue.TryPop())
				{
					std::lock_guard lock(resultsMutex);
					results.push_back(*val);
					consumed.fetch_add(1, std::memory_order_relaxed);
				}
			}
		});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	EXPECT_EQ(totalItems, results.size());
	std::sort(results.begin(), results.end());
	for (size_t i = 0; i < totalItems; ++i)
	{
		EXPECT_EQ(static_cast<int>(i), results[i]);
	}
}

TEST(ThreadSafeQueueTest, SwapExchangesContentAndCapacity)
{
	ThreadSafeQueue<int> queue1(10);
	ThreadSafeQueue<int> queue2(20);

	queue1.Push(1);
	queue1.Push(2);
	queue2.Push(3);

	queue1.Swap(queue2);

	EXPECT_EQ(20, queue1.GetCapacity());
	EXPECT_EQ(10, queue2.GetCapacity());

	int val;
	EXPECT_TRUE(queue1.TryPop(val));
	EXPECT_EQ(3, val);
	EXPECT_TRUE(queue1.IsEmpty());
	EXPECT_TRUE(queue2.TryPop(val));
	EXPECT_EQ(1, val);
	EXPECT_TRUE(queue2.TryPop(val));
	EXPECT_EQ(2, val);
	EXPECT_TRUE(queue2.IsEmpty());
}

TEST(ThreadSafeQueueTest, SwapNoDeadlockWithConcurrentAccess)
{
	ThreadSafeQueue<int> queue1(100);
	ThreadSafeQueue<int> queue2(100);

	for (int i = 0; i < 50; ++i)
	{
		queue1.Push(i);
		queue2.Push(i + 100);
	}

	std::jthread t1([&] { queue1.Swap(queue2); });
	std::jthread t2([&] { queue2.Swap(queue1); });

	t1.join();
	t2.join();

	EXPECT_EQ(100, queue1.GetSize() + queue2.GetSize());
}

TEST(ThreadSafeQueueTest, SwapWithDequeExchangesBuffer)
{
	ThreadSafeQueue<int> queue(10);
	queue.Push(10);
	queue.Push(20);

	std::deque external{ 30, 40, 50 };

	queue.Swap(external);

	EXPECT_EQ(3, queue.GetSize());
	EXPECT_EQ(2, external.size());

	int val;
	EXPECT_TRUE(queue.TryPop(val));
	EXPECT_EQ(30, val);
	EXPECT_TRUE(queue.TryPop(val));
	EXPECT_EQ(40, val);
	EXPECT_TRUE(queue.TryPop(val));
	EXPECT_EQ(50, val);

	EXPECT_EQ(10, external[0]);
	EXPECT_EQ(20, external[1]);
}

TEST(ThreadSafeQueueTest, SwapWithDequeAllowsSubsequentOperations)
{
	ThreadSafeQueue<int> queue(5);
	std::deque deque{ 1, 2, 3 };

	queue.Swap(deque);

	queue.Push(4);
	int val;
	EXPECT_TRUE(queue.TryPop(val));
	EXPECT_EQ(1, val);
	EXPECT_TRUE(queue.TryPop(val));
	EXPECT_EQ(2, val);
	EXPECT_TRUE(queue.TryPop(val));
	EXPECT_EQ(3, val);
	EXPECT_TRUE(queue.TryPop(val));
	EXPECT_EQ(4, val);
}

TEST(ThreadSafeQueueTest, WaitAndPopReturnsByValueForNothrowType)
{
	ThreadSafeQueue<int> queue;
	std::jthread producer([&] {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		queue.Push(123);
	});

	const int val = queue.WaitAndPop();
	EXPECT_EQ(123, val);
	producer.join();
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}