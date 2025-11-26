#include "MemoryManager.h"

#include <algorithm>
#include <cstring>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

class MemoryManagerTest : public ::testing::Test
{
protected:
	static constexpr size_t BUFFER_SIZE = 4096;
	alignas(std::max_align_t) char buffer[BUFFER_SIZE]{};
};

class MemoryManagerThreadSafetyTest : public ::testing::Test
{
protected:
	static constexpr size_t BUFFER_SIZE = 4 * 1024 * 1024;
	alignas(std::max_align_t) static char buffer[BUFFER_SIZE];
};

alignas(std::max_align_t) char MemoryManagerThreadSafetyTest::buffer[BUFFER_SIZE];

TEST_F(MemoryManagerTest, EmptyBuffer)
{
	MemoryManager mm(nullptr, 0);
	EXPECT_EQ(mm.Allocate(1), nullptr);
}

TEST_F(MemoryManagerTest, TooSmallBuffer)
{
	alignas(std::max_align_t) char smallBuf[sizeof(MemoryManager::BlockHeader) - 1];
	MemoryManager mm(smallBuf, sizeof(smallBuf));
	EXPECT_EQ(mm.Allocate(1), nullptr);
}

TEST_F(MemoryManagerTest, BasicAllocationAndFree)
{
	MemoryManager mm(buffer, BUFFER_SIZE);

	void* p1 = mm.Allocate(100);
	ASSERT_NE(p1, nullptr);
	void* p2 = mm.Allocate(200);
	ASSERT_NE(p2, nullptr);

	mm.Free(p1);
	mm.Free(p2);

	void* p3 = mm.Allocate(300);
	EXPECT_NE(p3, nullptr);
	mm.Free(p3);
}
TEST_F(MemoryManagerTest, AlignmentWorks)
{
	MemoryManager mm(buffer, BUFFER_SIZE);

	void* p1 = mm.Allocate(8, 8);
	ASSERT_NE(p1, nullptr);
	EXPECT_EQ(reinterpret_cast<uintptr_t>(p1) % 8, 0);

	void* p2 = mm.Allocate(16, 16);
	ASSERT_NE(p2, nullptr);
	EXPECT_EQ(reinterpret_cast<uintptr_t>(p2) % 16, 0);

	void* p3 = mm.Allocate(32, 32);
	ASSERT_NE(p3, nullptr);
	EXPECT_EQ(reinterpret_cast<uintptr_t>(p3) % 32, 0);

	mm.Free(p1);
	mm.Free(p2);
	mm.Free(p3);
}

TEST_F(MemoryManagerTest, InvalidAlignment)
{
	MemoryManager mm(buffer, BUFFER_SIZE);
	EXPECT_EQ(mm.Allocate(10, 3), nullptr);
	EXPECT_EQ(mm.Allocate(10, 0), nullptr);
}

TEST_F(MemoryManagerTest, Coalescing)
{
	MemoryManager mm(buffer, BUFFER_SIZE);

	void* p1 = mm.Allocate(100);
	void* p2 = mm.Allocate(100);
	void* p3 = mm.Allocate(100);
	ASSERT_NE(p1, nullptr);
	ASSERT_NE(p2, nullptr);
	ASSERT_NE(p3, nullptr);

	mm.Free(p2);
	mm.Free(p1);
	mm.Free(p3);

	void* big = mm.Allocate(300);
	EXPECT_NE(big, nullptr);
	mm.Free(big);
}

TEST_F(MemoryManagerTest, ZeroSizeAllocation)
{
	MemoryManager mm(buffer, BUFFER_SIZE);
	void* p = mm.Allocate(0);
	mm.Free(p);
}

TEST_F(MemoryManagerThreadSafetyTest, ConcurrentAllocateAndFree)
{
	MemoryManager mm(buffer, BUFFER_SIZE);

	const int numThreads = 4;
	const int opsPerThread = 500;

	std::vector<std::thread> threads;
	std::vector<std::vector<void*>> allocations(numThreads);

	for (int tid = 0; tid < numThreads; ++tid)
	{
		threads.emplace_back([&, tid]() {
			for (int i = 0; i < opsPerThread; ++i)
			{
				size_t size = 64 + (i % 128);
				void* ptr = mm.Allocate(size);
				if (ptr)
				{
					allocations[tid].push_back(ptr);
					std::memset(ptr, static_cast<int>(tid), size);
				}
			}
		});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	for (auto& allocs : allocations)
	{
		for (void* p : allocs)
		{
			mm.Free(p);
		}
	}

	void* big = mm.Allocate(BUFFER_SIZE / 2);
	ASSERT_NE(big, nullptr);
	mm.Free(big);
}